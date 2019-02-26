local Mrg = require('mrg')
local M = {}
local S = require('syscall')

function M.serialize (o)
  if type(o)     == "number" then return o
  elseif type(o) == "nil" then return "nil"
  elseif type(o) == "boolean" then
    if o == true then
      return "true" else
      return "false"
    end
  elseif type(o) == "string" then return string.format("%q", o)
  elseif type(o) == "table" then
    local ret = "{"
    for k,v in pairs(o) do
      ret = ret .. k .. '=' .. M.serialize(v) .. ','
    end
    ret = ret .. '}'
    return ret
  else
    error("cannot serialize a " .. type(o))
  end
end


---------------

local in_modal = false
local modal_x, modal_y = 100, 100
local modal_choices={}
local modal_string=nil

function M.modal(mrg, x, y, choices)
  if choices then
    modal_choices = choices
  else
    modal_choices = {}
  end
  local w, h = 100, 100
  if x < w/2 then x = w/2 end
  if y < h/2 then y = h/2 end
  if x > mrg:width()  - w/2 then x = mrg:width ()-w/2 end
  if y > mrg:height() - h/2 then y = mrg:height()-h/2 end
  modal_x  = x
  modal_y  = y
  in_modal = true
  modal_string = nil
  mrg:set_cursor_pos(0)
  mrg:queue_draw(nil)
end

function M.modal_end(mrg)
  in_modal = false
  mrg:queue_draw(nil)
end

function M.modal_draw(mrg)
  if not in_modal then return end

  local cr = mrg:cr()
  -- block out all other events, making clicking outside cancel

  cr:rectangle (0,0,mrg:width(),mrg:height())
  cr:set_source_rgba(0,0,0,0.5)
  mrg:listen(Mrg.COORD, function(event)
    event:stop_propagate()
  end)
  mrg:listen(Mrg.TAP, function(event)
    M.modal_end (mrg)
  end)
  cr:fill()
  
  cr:rectangle (modal_x - 50, modal_y - 50, 100, 100)
  cr:set_source_rgba(0,0,0, 0.5)
  mrg:listen(Mrg.COORD, function(event)
    event:stop_propagate()
  end)
  cr:fill()

  mrg:set_edge_left(modal_x - 50)
  mrg:set_edge_top(modal_y - 50)
  mrg:set_style('background:transparent; color: white; ')

  for i,v in pairs(modal_choices) do
    mrg:text_listen(Mrg.TAP, function(event)
      if v.cb and v.type ~= 'edit' then 
        M.modal_end (mrg)
        v.cb() 
      end
      mrg:queue_draw(nil)
    end)

    if v.type == 'edit' then
      mrg:edit_start(function(new_text)
        modal_string = new_text
      end)
      
      if modal_string then
        mrg:print(modal_string)
      else
        mrg:print(v.title )
      end

      mrg:edit_end()  

      mrg:add_binding("return", NULL, NULL, 
        function (event)
          if modal_string then
            v.cb (modal_string) -- print ('return!!')
            modal_string = nil
          end
          event:stop_propagate()
        end)
    else
      mrg:print(v.title )
    end
    mrg:print("\n")

    mrg:text_listen_done ()
  end
end


function M.which(command)
  local ret = ''
  local PATH = '.:' .. S.getenv('PATH') 
  for str in string.gmatch(PATH, "([^:]+)") do
    if S.stat(str .. '/' .. command) then
      local cmpstr = str
      if cmpstr == '.' then cmpstr = S.getcwd() end
      return cmpstr .. '/' .. command
    end
  end
  return nil
end


function string.has_suffix(String,End)
   return End=='' or string.sub(String,-string.len(End))==End
end
function string.has_prefix(String,Start)
     return string.sub(String,1,string.len(Start))==Start
end

return M
