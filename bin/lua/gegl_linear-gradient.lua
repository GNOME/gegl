
local start_x = active:get_property("start-x").value
local start_y = active:get_property("start-y").value
local end_x = active:get_property("end-x").value
local end_y = active:get_property("end-y").value

touch_point(start_x, start_y)
mrg:listen(Mrg.DRAG, function(ev)
  if ev.type == Mrg.DRAG_MOTION then
    local start_x = active:get_property("start-x").value
    local start_y = active:get_property("start-y").value
    start_x = start_x + ev.delta_x
    start_y = start_y + ev.delta_y
    active:set_property("start-y", GObject.Value(GObject.Type.DOUBLE, start_y))
    active:set_property("start-x", GObject.Value(GObject.Type.DOUBLE, start_x))
    ev:stop_propagate();
  end
end)

touch_point(end_x, end_y)
mrg:listen(Mrg.DRAG, function(ev)
  if ev.type == Mrg.DRAG_MOTION then
    local end_x = active:get_property("end-x").value
    local end_y = active:get_property("end-y").value
    end_x = end_x + ev.delta_x
    end_y = end_y + ev.delta_y
    active:set_property("end-y", GObject.Value(GObject.Type.DOUBLE, end_y))
    active:set_property("end-x", GObject.Value(GObject.Type.DOUBLE, end_x))
    ev:stop_propagate();
  end
end)
cr:new_path()

