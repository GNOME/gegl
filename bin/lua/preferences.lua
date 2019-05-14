
local em = mrg:em()
mrg:set_edge_left(mrg:height() * .2)

mrg:set_style("font-size: 10vh; text-stroke-width: 0.2em; text-stroke-color:rgba(0,0,0,0.4);color:white;background:transparent;");
mrg:print("\n");
mrg:print("Preferences\n");
mrg:set_style("font-size: 6vh");


mrg:text_listen(Mrg.PRESS, function(ev)
  if o.nearest_neighbor ~= 0 then
    o.nearest_neighbor = 0
  else
    o.nearest_neighbor = 1
  end
  ev:stop_propagate()
end)
mrg:print("nearest neighbour")
if o.nearest_neighbor ~= 0 then
  mrg:print(" yes")
else
  mrg:print(" no")
end
mrg:print("\n")
mrg:text_listen_done ()





mrg:text_listen(Mrg.PRESS, function(ev)
  if Gegl.config().mipmap_rendering then
    Gegl.config().mipmap_rendering = false
  else
    Gegl.config().mipmap_rendering = true
  end
  ev:stop_propagate()
end)
mrg:print("mipmap")
if Gegl.config().mipmap_rendering then
  mrg:print(" yes")
else
  mrg:print(" no")
end
mrg:print("\n")
mrg:text_listen_done ()


mrg:text_listen(Mrg.PRESS, function(ev)
  if Gegl.config().use_opencl then
    Gegl.config().use_opencl = false
  ffi.C.argvs_eval("clear")
  else
    Gegl.config().use_opencl = true
  ffi.C.argvs_eval("clear")
  end
  ev:stop_propagate()
end)
mrg:print("OpenCL")
if Gegl.config().use_opencl then
  mrg:print(" yes")
  ffi.C.argvs_eval("clear")
else
  mrg:print(" no")
  ffi.C.argvs_eval("clear")
end
mrg:text_listen_done ()
mrg:print("\n")


mrg:text_listen(Mrg.PRESS, function(ev)
  if o.color_managed_display ~= 0 then
    o.color_managed_display = 0
  else
    o.color_managed_display = 1
  end
  ev:stop_propagate()
end)
mrg:print("use display ICC profile")
if o.color_managed_display ~= 0 then
  mrg:print(" yes")
  ffi.C.argvs_eval("clear")
else
  mrg:print(" no")
  ffi.C.argvs_eval("clear")
end
mrg:text_listen_done ()
mrg:print("\n")


mrg:text_listen(Mrg.PRESS, function(ev)
  if o.frame_cache ~= 0 then
    o.frame_cache = 0
  else
    o.frame_cache = 1
  end
  ev:stop_propagate()
end)
mrg:print("frame caching")
if o.frame_cache ~= 0 then
  mrg:print(" yes")
else
  mrg:print(" no")
end
mrg:print("\n")
mrg:text_listen_done ()


mrg:set_style("font-size: 3vh");

mrg:print("threads: " .. Gegl.config().threads .. "\n")
mrg:print("tile-size: " .. Gegl.config().tile_width .. 'x' .. Gegl.config().tile_height .."\n")
mrg:print("tile-cache-size: " .. (Gegl.config().tile_cache_size / 1024 / 1024) .. 'mb\n')
mrg:print("swap-compression: " .. Gegl.config().swap_compression .. "\n")
mrg:print("swap: " .. Gegl.config().swap .. "\n")
mrg:print("max file backend write queue-size: " .. (Gegl.config().queue_size / 1024/1024 ) .. "mb\n")
mrg:print("chunk-size: " .. (Gegl.config().chunk_size / 1024 / 1024) .. 'mb\n')
mrg:print("application-license: " .. Gegl.config().application_license.. "\n")




