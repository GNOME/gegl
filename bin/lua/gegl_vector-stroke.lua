local path = active:get_property("d").value

-- there seem to be enough exposed through gobject introspection
-- to build a full bezier editor, for now placeholder.

--print ('\n nodes:' .. path:get_n_nodes())
--print ('length:' .. path:get_length())
--local a = path:get_node(0)
--print ('a:' .. a.type)


local no = 0

path:foreach_flat( function(el)
  if el.type == 77 then
    cr:move_to(el.point[1].x, el.point[1].y)
  elseif el.type == 76 then
    cr:line_to(el.point[1].x, el.point[1].y)
  elseif el.type == 122 then
  end
end)

cr:set_line_width(dim/20)
cr:stroke()

-- 67 : C
-- 76 : L
-- 77 : M
-- 99 : c
-- 108 : l
-- 109 : m
-- 122 : z

local cur_x = 0
local cur_y = 0

path:foreach( function(el)

  local coord_no = 1

  if el.type == 67 then -- 'C'

    for coord_no = 1, 3 do
    local x = el.point[coord_no].x
    local y = el.point[coord_no].y

    cr:new_path()
    cr:move_to (el.point[3].x, el.point[3].y)
    cr:line_to (el.point[2].x, el.point[2].y)
    cr:stroke()


    cr:new_path()
    cr:move_to (el.point[1].x, el.point[1].y)
    cr:line_to (cur_x, cur_y)
    cr:stroke()


    cr:new_path()
    cr:arc(x, y, dim/15, 0, 3.1415 * 2)
    mrg:listen(Mrg.DRAG, function(ev)
      if ev.type == Mrg.DRAG_MOTION then

        if coord_no == 3 then
          el.point[3].x = el.point[3].x + ev.delta_x
          el.point[3].y = el.point[3].y + ev.delta_y

          el.point[2].x = el.point[2].x + ev.delta_x
          el.point[2].y = el.point[2].y + ev.delta_y

          -- todo also drag part of previous if it indeed is a curve
        else
          el.point[coord_no].x = el.point[coord_no].x + ev.delta_x
          el.point[coord_no].y = el.point[coord_no].y + ev.delta_y
        end

        path:dirty()
        ev:stop_propagate();
      end
    end)
    cr:set_source_rgb(1,0,0)
    cr:fill()
    end

    coord_no = 3
  elseif el.type == 122 then
  else
    coord_no = 1
    local x = el.point[coord_no].x
    local y = el.point[coord_no].y
    cr:new_path()
    cr:arc(x, y, dim/15, 0, 3.1415 * 2)
    mrg:listen(Mrg.DRAG, function(ev)
      if ev.type == Mrg.DRAG_MOTION then
        el.point[coord_no].x = el.point[coord_no].x + ev.delta_x
        el.point[coord_no].y = el.point[coord_no].y + ev.delta_y

        path:dirty()
        ev:stop_propagate();
      end
    end)
    cr:set_source_rgb(1,0,0)
    cr:fill()
  end


  cur_x = el.point[coord_no].x
  cur_y = el.point[coord_no].y
 --print ('' .. no .. ' - ' .. el.type )

  no = no + 1
end)
