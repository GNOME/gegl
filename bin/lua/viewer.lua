-- mrg ui for image viewer mode of gegl ui

-- todo : implement thumb-bar of previous/next image
--        timeout for showing of controls


cr:new_path()
cr:rectangle(0,0,mrg:width(),mrg:height())

if zoom_pinch_coord == nil then
  zoom_pinch_coord = {{},{},{},{}}
end

mrg:listen(Mrg.DRAG, function(event)
  if event.type == Mrg.DRAG_PRESS then

    if event.device_no == 5 then -- 5 = second finger
      zoom_pinch_coord[2].x = event.x;
      zoom_pinch_coord[2].y = event.y;

      zoom_pinch_coord[3].x = zoom_pinch_coord[1].x;
      zoom_pinch_coord[3].y = zoom_pinch_coord[1].y;
      zoom_pinch_coord[4].x = zoom_pinch_coord[2].x;
      zoom_pinch_coord[4].y = zoom_pinch_coord[2].y;

      zoom_pinch = 1
      orig_zoom = o.scale
    elseif event.device_no == 1 or event.device_no == 4 then -- mouse or first finger
      zoom_pinch_coord[1].x = event.x;
      zoom_pinch_coord[1].y = event.y;

    end
  elseif event.type == Mrg.DRAG_MOTION then
    if event.device_no == 1 or event.device_no == 4 then
      zoom_pinch_coord[1].x = event.x;
      zoom_pinch_coord[1].y = event.y;
    end
    if event.device_no == 5 then
      zoom_pinch_coord[2].x = event.x;
      zoom_pinch_coord[2].y = event.y;
    end

    if zoom_pinch ~= nil then
      local orig_dist = ffi.C.hypotf (zoom_pinch_coord[3].x - zoom_pinch_coord[2].x ,
                                      zoom_pinch_coord[3].y - zoom_pinch_coord[2].y );
      local dist = ffi.C.hypotf (zoom_pinch_coord[1].x - zoom_pinch_coord[2].x ,
                                 zoom_pinch_coord[1].y - zoom_pinch_coord[2].y);
      local x, y;
      local screen_cx = (zoom_pinch_coord[1].x + zoom_pinch_coord[2].x)/2;
      local screen_cy = (zoom_pinch_coord[1].y + zoom_pinch_coord[2].y)/2;

      local x = (o.u + screen_cx) / o.scale
      local y = (o.v + screen_cy) / o.scale

      o.scale = orig_zoom * dist / orig_dist;
      o.u = x * o.scale - screen_cx;
      o.v = y * o.scale - screen_cy;

      o.u = o.u - (event.delta_x )/2
      o.v = o.v - (event.delta_y )/2
      o.is_fit = 0;
    else
      o.u = o.u - event.delta_x
      o.v = o.v - event.delta_y
    end
  elseif event.type == Mrg.DRAG_RELEASE then
    zoom_pinch = nil
  end
  event:stop_propagate()
end)




mrg:listen(Mrg.MOTION, function(event)
  o.show_controls = 1
  end)

mrg:listen(Mrg.SCROLL, function(event)
  local factor = 1.05
  local x = (o.u + event.device_x) / o.scale
  local y = (o.v + event.device_y) / o.scale

  if event.scroll_direction == Mrg.SCROLL_DIRECTION_DOWN then
    o.scale = o.scale * factor
  elseif event.scroll_direction == Mrg.SCROLL_DIRECTION_UP then
    o.scale = o.scale / factor
  end

  o.u = x * o.scale - event.device_x
  o.v = y * o.scale - event.device_y
  o.is_fit = 0
  mrg:queue_draw(nil)
end
)

function draw_grid(x,y,w,h)
  cr:new_path()
  cr:rectangle (0.00 *w + x, 0.00 * h + y, 0.33 * w, 0.33 * h)
  cr:rectangle (0.66 *w + x, 0.00 * h + y, 0.33 * w, 0.33 * h)
  cr:rectangle (0.00 *w + x, 0.66 * h + y, 0.33 * w, 0.33 * h)
  cr:rectangle (0.66 *w + x, 0.66 * h + y, 0.33 * w, 0.33 * h)
end

function draw_back(x,y,w,h)
  cr:new_path ();
  cr:new_path ();
  cr:move_to (x+0.9*w, y+0.1*h);
  cr:line_to (x+0.9*w, y+0.9*h);
  cr:line_to (x+0.1*w, y+0.5*h);
end

function draw_forward(x,y,w,h)
  cr:new_path ();
  cr:move_to (x+0.1*w, y+0.1*h);
  cr:line_to (x+0.1*w, y+0.9*h);
  cr:line_to (x+0.9*w, y+0.5*h);
end

function draw_edit(x,y,w,h)
  cr:new_path ();
  cr:arc (x+0.5*w, y+0.5*h, h * .4, 0.0, 3.1415 * 2);
end

function contrasty_stroke()
  local x0 = 6.0
  local y0 = 6.0;
  local x1 = 4.0
  local y1 = 4.0;

  x0, y0 = cr:device_to_user_distance (x0, y0)
  x1, y1 = cr:device_to_user_distance (x1, y1)
  cr:set_source_rgba (0,0,0,0.5);
  cr:set_line_width (y0);
  cr:stroke_preserve ()
  cr:set_source_rgba (1,1,1,0.5);
  cr:set_line_width (y1);
  cr:stroke ();
end

-- ui_viewer

cr:save()
cr:rectangle(0,0,mrg:width(),mrg:height())

local width = mrg:width()
local height = mrg:height()

draw_grid(height* 0.1/4, height * 0.1/4, height * 0.1, height *0.1)

if o.show_controls ~= 0 then
  contrasty_stroke()
else
  cr:new_path()
end

cr:rectangle (0, 0, height * 0.15, height * 0.15);

if o.show_controls ~= 0 then
  cr:set_source_rgba (1,1,1,.1);
  cr:fill_preserve ();
end

mrg:listen(Mrg.PRESS, function(event) ffi.C.argvs_eval('parent') end)

draw_back (height * .1 / 4, height * .5, height * .1, height *.1);
cr:close_path ();

if o.show_controls ~= 0 then
  contrasty_stroke ();
else
  cr:new_path ();
end

cr:rectangle (0, height * .3, height * .15, height *.7);

if o.show_controls ~= 0 then
  cr:set_source_rgba (1,1,1,.1);
  cr:fill_preserve ();
end

mrg:listen(Mrg.PRESS, function(event) ffi.C.argvs_eval('prev') end)
cr:new_path ();

draw_forward (width - height * .12, height * .5, height * .1, height *.1);
cr:close_path ();

if o.show_controls ~= 0 then
  contrasty_stroke ();
else
  cr:new_path ();
end

cr:rectangle (width - height * .15, height * .3, height * .15, height *.7);

if o.show_controls ~= 0 then
  cr:set_source_rgba (1,1,1,.1);
  cr:fill_preserve ();
end

mrg:listen(Mrg.PRESS, function(event) ffi.C.argvs_eval('next') end)


draw_edit (width - height * .15, height * .0, height * .15, height *.15);
if o.show_controls ~= 0 then
  contrasty_stroke ();
else
  cr:new_path ();
end

cr:rectangle (width - height * .15, height * .0, height * .15, height *.15);
if o.show_controls ~= 0 then
 cr:set_source_rgba (1,1,1,.1);
 cr:fill_preserve ();
end
mrg:listen(Mrg.PRESS, function(event) ffi.C.argvs_eval('toggle editing') end)
cr:new_path ();


mrg:add_binding("page-up", NULL, "previous image",
  function() ffi.C.argvs_val ("prev") end)

mrg:add_binding("page-down", NULL, "next image",
  function() ffi.C.argvs_val ("next") end)

mrg:add_binding("alt-right", NULL, "next image",
  function() ffi.C.argvs_val ("next") end)

mrg:add_binding("alt-left", NULL, "rev image",
  function() ffi.C.argvs_val ("prev") end)

mrg:add_binding("control-s", NULL, "toggle slideshow",
  function() ffi.C.argvs_val ("toggle slideshow") end)

mrg:add_binding("control-m", NULL, "toggle mipmap",
  function() ffi.C.argvs_val ("toggle mipmap") end)

mrg:add_binding("control-y", NULL, "toggle display profile",
  function() ffi.C.argvs_val ("toggle colormanaged-display") end)

mrg:add_binding("control-delete", NULL, "discard",
  function() ffi.C.argvs_val ("discard") end)
