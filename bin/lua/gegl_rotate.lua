local origin_x = active:get_property("origin-x").value
local origin_y = active:get_property("origin-y").value
local degrees =  active:get_property("degrees").value

local rotate_x = origin_x + (dim * 2) * math.cos (-degrees/360.0 * 3.141529 * 2)
local rotate_y = origin_y + (dim * 2) * math.sin (-degrees/360.0 * 3.141529 * 2)

cr:new_path()
cr:move_to(origin_x, origin_y)
cr:line_to(rotate_x, rotate_y)
contrasty_stroke()


touch_point(origin_x, origin_y)
mrg:listen(Mrg.DRAG, function(ev)
  if ev.type == Mrg.DRAG_MOTION then
    local x = active:get_property("origin-x").value
    local y = active:get_property("origin-y").value
    x = x + ev.delta_x
    y = y + ev.delta_y
    active:set_property("origin-y", GObject.Value(GObject.Type.DOUBLE, y))
    active:set_property("origin-x", GObject.Value(GObject.Type.DOUBLE, x))
    ev:stop_propagate();
  end
end)
cr:new_path()

touch_point(rotate_x, rotate_y)
mrg:listen(Mrg.DRAG, function(ev)
  if ev.type == Mrg.DRAG_MOTION then
    local new_degrees = math.atan2(ev.x-origin_x,ev.y-origin_y)*180/3.141529 - 90
    if new_degrees < 0 then new_degrees = new_degrees + 360 end
    active:set_property("degrees",
           GObject.Value(GObject.Type.DOUBLE, new_degrees))
    ev:stop_propagate();
  end
end)
cr:new_path()


