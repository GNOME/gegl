local x = active:get_property("x").value
local y = active:get_property("y").value
local w = active:get_property("width").value
local h = active:get_property("height").value

touch_point(x,y)
mrg:listen(Mrg.DRAG, function(ev)
  if ev.type == Mrg.DRAG_MOTION then
    local x = active:get_property("x").value
    local y = active:get_property("y").value
    local w = active:get_property("width").value
    local h = active:get_property("height").value
    x = x + ev.delta_x
    y = y + ev.delta_y
    w = w - ev.delta_x
    h = h - ev.delta_y
    active:set_property("y", GObject.Value(GObject.Type.DOUBLE, y))
    active:set_property("x", GObject.Value(GObject.Type.DOUBLE, x))
    active:set_property("width", GObject.Value(GObject.Type.DOUBLE, w))
    active:set_property("height", GObject.Value(GObject.Type.DOUBLE, h))
    ev:stop_propagate();
  end
end)

touch_point(x+w,y+h)
mrg:listen(Mrg.DRAG, function(ev)
  if ev.type == Mrg.DRAG_MOTION then
    local w = active:get_property("width").value
    local h = active:get_property("height").value
    w = w + ev.delta_x
    h = h + ev.delta_y
    active:set_property("width", GObject.Value(GObject.Type.DOUBLE, w))
    active:set_property("height", GObject.Value(GObject.Type.DOUBLE, h))
    ev:stop_propagate();
  end
end)
cr:new_path()

