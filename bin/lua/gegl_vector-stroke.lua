local path = active:get_property("d").value

path:foreach( function(el)
  local x = el.point[1].x
  local y = el.point[1].y

  cr:new_path()
  cr:arc(x, y, dim/15, 0, 3.1415 * 2)

mrg:listen(Mrg.DRAG, function(ev)
  if ev.type == Mrg.DRAG_MOTION then
    local x = el.point[1].x
    local y = el.point[1].y
    x = x + ev.delta_x
    y = y + ev.delta_y
    el.point[1].x = x
    el.point[1].y = y
    path:dirty()
    ev:stop_propagate();
  end
end)

  cr:set_source_rgb(0,1,0)
  cr:fill()

end)
