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

