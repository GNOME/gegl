local std_dev = active:get_property("std-dev").value

touch_point(centerx - dim  - std_dev, centery)
mrg:listen(Mrg.DRAG, function(ev)
  if ev.type == Mrg.DRAG_MOTION then
    local x = active:get_property("std-dev").value
    x = x - ev.delta_x
    if x < 0 then x = 0 end
    active:set_property("std-dev", GObject.Value(GObject.Type.DOUBLE, x))
    ev:stop_propagate();
  end
end)
cr:new_path()

cr:arc(centerx, centery, std_dev, 0, 3.14152 * 2)
contrasty_stroke()
cr:arc(centerx, centery, std_dev * 2, 0, 3.14152 * 2)
contrasty_stroke()
