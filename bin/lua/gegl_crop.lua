local x = active:get_property("x").value
local y = active:get_property("y").value

cr:new_path()
cr:arc(x, y, dim/2, 0, 3.1415 * 2)

mrg:listen(Mrg.DRAG, function(ev)
  if ev.type == Mrg.DRAG_MOTION then
    local x = active:get_property("x").value
    local y = active:get_property("y").value
    x = x + ev.delta_x
    y = y + ev.delta_y
    active:set_property("y", GObject.Value(GObject.Type.DOUBLE, y))
    active:set_property("x", GObject.Value(GObject.Type.DOUBLE, x))
    ev:stop_propagate();
  end
end)

cr:set_source_rgba(1,0,0,0.5)
cr:fill()

local w = active:get_property("width").value
local h = active:get_property("height").value

cr:new_path()
cr:arc(x + w, y + h, dim/2, 0, 3.1415 * 2)
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
cr:set_source_rgba(1,0,0,0.5)
cr:fill()

