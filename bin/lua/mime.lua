local M = {}

-- the api is possible to replace with something
-- that sniffs magic, this is the simplest thing
-- that almost works though


function M.for_path(path)

if path:match("mrl-") then
  return "application/mmm"
end
if path:match("mmm-") then
  return "application/mmm"
end
if path:match("mrg-") then
  return "application/mmm"
end

   if path:has_suffix(".mp4") then
     return "video/mpeg4"
   elseif path:has_suffix(".ogv") then
     return "video/ogv"
   elseif path:has_suffix(".mpg") then
     return "video/mpeg"
   end

   if path:has_prefix("/proc/") then
     return "text/plain"
   elseif path:has_suffix(".gif") or
      path:has_suffix(".GIF") then
     return "image/gif";
   elseif path:has_suffix(".png") or 
      path:has_suffix(".PNG") then
      return "image/png"
   elseif path:has_suffix(".svg") or 
      path:has_suffix(".SVG") then
      return "image/svg"
   elseif path:has_suffix(".html") or 
      path:has_suffix(".HTML") then
      return "text/html"
   elseif path:has_suffix(".jpg") or
            path:has_suffix(".jpeg") or
            path:has_suffix(".JPEG") or
            path:has_suffix(".JPG") 
     then
       return "image/jpeg"
   elseif path:has_suffix("README") or 
      path:has_suffix(".txt") or
      path:has_suffix("Makefile") or
      path:has_suffix("Makefile.am") or
      path:has_suffix("configure") or
      path:has_suffix('.c') or 
      path:has_suffix('.js') or 
      path:has_suffix('.md') or 
      path:has_suffix('.pc') or 
      path:has_suffix('.inc') or 
      path:has_suffix('.h') or 
      path:has_suffix('.css') 
     -- or 
     -- path:has_suffix(".lua")
     then
      return "text/plain"
   end

  io.input(path)
  document = io.read("*line")
  io.close()

  if document and document:match("luajit") then
    return "application/mmm"
  end
  return "text/plain"
end

return M
