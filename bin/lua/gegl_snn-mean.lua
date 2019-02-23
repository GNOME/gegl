local radius = active:get_property("radius").value

touch_point(centerx - dim  - radius, centery)
mrg:listen(Mrg.DRAG, function(ev)
  if ev.type == Mrg.DRAG_MOTION then
    local x = active:get_property("radius").value
    x = x - ev.delta_x
    if x < 0 then x = 0 end
    active:set_property("radius", GObject.Value(GObject.Type.DOUBLE, x))
    ev:stop_propagate();
  end
end)
cr:new_path()

cr:arc(centerx, centery, radius, 0, 3.14152 * 2)
contrasty_stroke()
cr:arc(centerx, centery, radius * 2, 0, 3.14152 * 2)
contrasty_stroke()
