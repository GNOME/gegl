#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <mutex>

#include "zombie/zombie.hpp"
#include "gegl-zombie-manager.h"
#include "gegl-zombie-manager-private.h"
#include "gegl-cache.h"
#include "gegl-node-private.h"
#include "gegl-region.h"
#include "buffer/gegl-tile-storage.h"
#include "process/gegl-eval-manager.h"

// last lock in 2 phase locking
std::mutex zombie_mutex;
using lock_guard = std::lock_guard<std::mutex>;

std::string getEnvVar(const std::string& key) {
  char const* val = getenv(key.c_str());
  return val == nullptr ? std::string() : std::string(val);
}

bool use_zombie() {
  return getEnvVar("USE_ZOMBIE") == "1";
}

void gegl_zombie_link_test() {
  zombie_link_test();
}

using Key = std::tuple<gint, gint, gint>;

struct Proxy {
  size_t size;
  explicit Proxy(size_t size) : size(size) { }
  Proxy() = delete;
};

template<>
struct GetSize<Proxy> {
  size_t operator()(const Proxy& p) {
    return p.size;
  }
};
// A Zombified Tile.
using ZombieTile = Zombie<Proxy>;

std::ostream& operator<<(std::ostream& os, const GeglRectangle& rect) {
  os << "(x:[" << rect.x << ", " << rect.x + rect.width << "), y:[" << rect.y << ", " << rect.y + rect.height << "))";
  return os;
}

namespace std {
namespace {

// Code from boost
// Reciprocal of the golden ratio helps spread entropy
//     and handles duplicates.
// See Mike Seymour in magic-numbers-in-boosthash-combine:
//     http://stackoverflow.com/questions/4948780

template <class T>
inline void hash_combine(std::size_t& seed, T const& v) {
  seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

// Recursive template code derived from Matthieu M.
template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
struct HashValueImpl {
  static void apply(size_t& seed, Tuple const& tuple) {
    HashValueImpl<Tuple, Index-1>::apply(seed, tuple);
    hash_combine(seed, std::get<Index>(tuple));
  }
};

template <class Tuple>
struct HashValueImpl<Tuple,0> {
  static void apply(size_t& seed, Tuple const& tuple) {
    hash_combine(seed, std::get<0>(tuple));
  }
};
}

template <typename ... TT>
struct hash<std::tuple<TT...>> {
  size_t operator()(std::tuple<TT...> const& tt) const {
    size_t seed = 0;
    HashValueImpl<std::tuple<TT...> >::apply(seed, tt);
    return seed;
  }
};
}

inline std::string TileCommandName(GeglTileCommand command) {
  switch (command) {
  case GEGL_TILE_IDLE:
    return "zombie_command_idle";
  case GEGL_TILE_GET:
    return "zombie_command_get_zombies";
  case GEGL_TILE_SET:
    return "zombie_command_set";
  case GEGL_TILE_IS_CACHED:
    return "zombie_tile_is_cached";
  case GEGL_TILE_EXIST:
    return "zombie_command_exist";
  case GEGL_TILE_VOID:
    return "zombie_command_void";
  case GEGL_TILE_FLUSH:
    return "zombie_command_flush";
  case GEGL_TILE_REFETCH:
    return "zombie_command_refetch";
  case GEGL_TILE_REINIT:
    return "zombie_command_reinit";
  case GEGL_TILE_COPY:
    return "zombie_command_copy";
  default:
    return "unknown";
  }
}

bool operator==(const GeglRectangle& lhs, const GeglRectangle& rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.width == rhs.width && lhs.height == rhs.height;
}

// Every tile of the buffer that zombie see is categorized into 3 type:
// A Zombie, which is a tile that is being managed normally,
// A PreZombie, which is a tile that had been freshly recorded, waiting to be commit in a bit,
// An Input, which mean this is a input of some sort, and Zombie should not manage it.
// Also - if my understanding is correct, all tile of the same buffer should be in the same type.
struct _GeglZombieManager {
  GeglNode* node;
  GWeakRef cache;
  bool initialized = false;
  std::optional<GeglRectangle> tile;
  std::unordered_map<Key, ZombieTile> map;
  std::mutex mutex;

  _GeglZombieManager(GeglNode* node) : node(node) {
    g_weak_ref_init(&cache, nullptr);
  }

  ~_GeglZombieManager() {
    g_clear_object (&node->cache);
    gpointer cache_strong = g_weak_ref_get(&cache);
    if (cache_strong != nullptr) {
      static_cast<GeglTileSource*>(cache_strong)->command = gegl_buffer_command;
      g_object_unref(cache_strong);
    }
    g_weak_ref_clear(&cache);
  }

  ZombieTile GetTile(const Key& k, const lock_guard& lg) {
    // A tile could be not in map, because we are overapproximating.
    // To be more precise, parent(over_approximate(r)) might be bigger then parent(r),
    // so the former will have more tile.
    // When that happend, we set the non-existent tile.
    // Why dont we set all the tile in the beginning instead?
    // Some gegl operation create a huge tile with only a portion of it being used.
    // If we manage all of those tile overhead will be unbearable.
    if (map.count(k) == 0) {
      SetTile(k, lg);
    }
    return map.at(k);
  }

  ZombieTile GetTile(const Key& k) {
    lock_guard lg(mutex);
    return GetTile(k, lg);
  }

  size_t GetTileSize() const {
    assert(tile);
    // 4 from rgba
    return tile.value().width * tile.value().height * 4;
  }

  ZombieTile MakeZombieTile(Key k) {
    lock_guard lg(zombie_mutex);
    // todo: calculate parent dependency
    auto tile_size = GetTileSize();
    if (node->cache != nullptr) {
      ZombieTile zt(bindZombie([tile_size](){ return ZombieTile(Proxy{tile_size}); }));
      zt.evict(); // doing a single eviction to make sure we can recompute
      return zt;
    } else {
      return bindZombie([tile_size](){
        ZombieTile zt(Proxy{tile_size});
        zt.evict();
        return zt;
      });
    }
  }

  void SetTile(const Key& k, const lock_guard& lg) {
    assert(map.count(k) == 0);
    map.insert({k, MakeZombieTile(k)});
  }

  void SetTile(const Key& k) {
    lock_guard lg(mutex);
    SetTile(k, lg);
  }

  // todo: avoid tile get reentrant
  gpointer tile_get(gint x, gint y, gint z, GeglTileGetState s) {
    auto forward = [&](){
      // why not gegl_node_get_cache? because if this is ever called cache had been set.
      return gegl_buffer_command (GEGL_TILE_SOURCE (node->cache), GEGL_TILE_GET, x, y, z, reinterpret_cast<gpointer>(s));
    };
    GeglRectangle tile = this->tile.value();
    Key k { tile.x + x * tile.width, tile.y + y * tile.height, z };
    switch (s) {
    case GEGL_TILE_GET_SENTRY:
      std::cout << "GEGL TILE GET SENTRY!";
      assert(false);
      exit(1);
    case GEGL_TILE_GET_READ: {
      bool should_work = [&](){
        lock_guard lg(mutex);
        return (map.count(k) != 0) && (GetTile(k, lg).evicted());
      }();
      if (should_work) {
        GeglRectangle tile = this->tile.value();
        GeglRectangle roi = *GEGL_RECTANGLE(std::get<0>(k), std::get<1>(k), tile.width, tile.height);
        {
          GeglRegion *temp_region = gegl_region_rectangle (&roi);
          g_mutex_lock(&node->cache->mutex);
          gegl_region_subtract(node->cache->valid_region[z], temp_region);
          g_mutex_unlock(&node->cache->mutex);
          gegl_region_destroy(temp_region);
        }
        // this is being called by some code which lock the cache.
        // recomputing will write to cache, which will also aquire the lock,
        // so it will dead lock.
        // since we are not touching the cache here,
        // we can revert the lock.
        // may god forgive my sin.
        g_rec_mutex_unlock(&GEGL_BUFFER(node->cache)->tile_storage->mutex);
        GeglEvalManager * em = gegl_eval_manager_new(node, "output");
        gegl_eval_manager_recompute(em);
        gegl_eval_manager_apply(em, &roi, z);
        g_rec_mutex_lock(&GEGL_BUFFER(node->cache)->tile_storage->mutex);
        gegl_cache_computed(node->cache, &roi, z);
        {
          lock_guard lg(mutex);
          lock_guard zombie_lg(zombie_mutex);
          GetTile(k, lg).recompute();
        }
      }
    }
    case GEGL_TILE_GET_PARTIAL_WRITE:
      return forward();
    case GEGL_TILE_GET_FULL_WRITE:
      return forward();
    };
    return forward();
  }

  // TODO: I dont think the handling of level is correct
  gpointer command(GeglTileCommand   command,
                   gint              x,
                   gint              y,
                   gint              z,
                   gpointer          data) {
    auto forward = [&](){
      // why not gegl_node_get_cache? because if this is ever called cache had been set.
      return gegl_buffer_command (GEGL_TILE_SOURCE (node->cache), command, x, y, z, data);
    };

    if ((!use_zombie()) || (!initialized)) {
      return forward();
    } else {
      switch (command) {
      case GEGL_TILE_GET: {
        return tile_get(x, y, z, *reinterpret_cast<GeglTileGetState*>(&data));
        break;
      }
      case GEGL_TILE_IS_CACHED:
        // todo: maybe implement.
        // no code seems to rely on this though.
        return forward();
      default:
        break;
      }
      std::cout << TileCommandName(command) << std::endl;
      assert(false);
      return forward();
    }
  }

  void prepare() {
    // todo: we want to record time here
  }

  std::vector<GeglRectangle> SplitToTiles(const GeglRectangle& roi) const {
    assert(initialized);
    std::vector<GeglRectangle> ret;
    if (this->tile) {
      auto tile = this->tile.value();
      GeglRectangle rect;
      gegl_rectangle_align(&rect, &roi, &tile, GEGL_RECTANGLE_ALIGNMENT_SUPERSET);
      for (gint x = rect.x; x < rect.x + rect.width; x += tile.width) {
        for (gint y = rect.y; y < rect.y + rect.height; y += tile.height) {
          assert(y + tile.height <= rect.y + rect.height);
          GeglRectangle one = *GEGL_RECTANGLE(x, y, tile.width, tile.height);
          ret.push_back(one);
        }
        // y + tile.height == rect.y + rect.height
        assert(x + tile.width <= rect.x + rect.width);
      }
      // x + tile.width == rect.x + rect.width
    }
    return ret;
  }

  void commit(const GeglRectangle& roi,
	      GeglBuffer* buffer,
	      gint level) {
    if (use_zombie()) {
      std::optional<GeglRectangle> tile;
      if (buffer != nullptr) {
        tile = *GEGL_RECTANGLE (buffer->shift_x,
                                buffer->shift_y,
                                buffer->tile_width,
                                buffer->tile_height);
      }
      lock_guard lg(mutex);
      if (!initialized) {
        if (false) {
          std::cout << "name: " << std::string(gegl_node_get_operation(node)) << std::endl;
          std::cout << "cache:" << ((node->cache != nullptr) ? "yes" : "no") << std::endl;
          std::cout << "bb:   " << gegl_node_get_bounding_box(node) << std::endl;
          std::cout << "roi:  " << roi << std::endl;
        }
        initialized = true;
        this->tile = tile;
      }
      for (const GeglRectangle& r: SplitToTiles(roi)) {
        // todo: we may want more fine grained tracking
        GetTile({r.x, r.y, level}, lg);
      }
      assert(this->tile == tile);
    }
  }
};

GeglZombieManager* make_zombie_manager(GeglNode* node) {
  return new GeglZombieManager(node);
}

void destroy_zombie_manager(GeglZombieManager* m) {
  delete m;
}

gpointer zombie_manager_command (GeglZombieManager *self,
				 GeglTileCommand   command,
				 gint              x,
				 gint              y,
				 gint              z,
				 gpointer          data) {
  return self->command(command, x, y, z, data);
}

void zombie_manager_set_cache (GeglZombieManager *self,
			       GeglCache* cache) {
  // cachiness should not change as we depend on it.
  assert(!self->initialized);
  g_weak_ref_set(&(self->cache), cache);
}

void zombie_manager_prepare(GeglZombieManager* self) {
  self->prepare();
}

void zombie_manager_commit(GeglZombieManager*   self,
                           GeglBuffer*          buffer,
                           const GeglRectangle* roi,
			   gint                 level) {
  self->commit(*roi, buffer, level);
}
