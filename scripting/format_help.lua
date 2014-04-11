-- RST
-- format_help.lua
-- ---------------

-- Functions used in the ingame help windows for formatting the text and pictures.

-- RST
-- TODO remove this once everything works
-- .. function:: dependencies(images[, text = nil])
--
--    Creates a dependencies line of any length.
--
--    :arg images: images in the correct order from left to right as table (set in {}).
--    :arg text: comment of the image.
--    :returns: a row of pictures connected by arrows.
--
function dependencies_old(images, text)
	if not text then
		text = ""
	end

	string = "image=" .. images[1]
	for k,v in ipairs({table.unpack(images,2)}) do
		string = string .. ";pics/arrow-right.png;" .. v
	end

	return rt(string, text)
end

--
-- Functions used in the ingame help windows for formatting the text and pictures.

-- RST
-- .. function:: dependencies(tribename, items[, text = nil])
--
--    Creates a dependencies line of any length.
--
--    :arg tribename: name of the tribe.
--    :arg items: wares and/or buildings in the correct order from left to right as table (set in {}).
--    :arg text: comment of the image.
--    :returns: a row of pictures connected by arrows.
--
function dependencies(tribename, items, text)
	if not text then
		text = ""
	end

	string = "image=tribes/" .. tribename .. "/" .. items[1]  .. "/menu.png"
	for k,v in ipairs({table.unpack(items,2)}) do
		string = string .. ";pics/arrow-right.png;" ..  "tribes/" .. tribename .. "/" .. v  .. "/menu.png"
	end
	return rt(string, p(text))
end


-- RST
-- .. function:: image_line(image, count[, text = nil])
--
--    Aligns the image to a row on the right side with text on the left.
--
--    :arg image: the picture to be aligned to a row.
--    :arg count: length of the picture row.
--    :arg text: if given the text aligned on the left side, formatted via
--       formatting.lua functions.
--    :returns: the text on the left and a picture row on the right.
--
function image_line(image, count, text)
	local imgs={}
	for i=1,count do
		imgs[#imgs + 1] = image
	end
	local imgstr = table.concat(imgs, ";")

	if text then
		return rt("image=" .. imgstr .. " image-align=right", "  " .. text)
	else
		return rt("image=" .. imgstr .. " image-align=right", "")
	end
end

-- RST
-- .. function text_line(t1, t2[, imgstr = nil])
--
--    Creates a line of h3 formatted text followed by normal text and an image.
--
--    :arg t1: text in h3 format.
--    :arg t2: text in p format.
--    :arg imgstr: image aligned right.
--    :returns: header followed by normal text and image.
--
function text_line(t1, t2, imgstr)
	if imgstr then
		return "<rt text-align=left image=" .. imgstr .. " image-align=right><p font-size=13 font-color=D1D1D1>" ..  t1 .. "</p><p line-spacing=3 font-size=12>" .. t2 .. "<br></p><p font-size=8> <br></p></rt>"
	else
		return "<rt text-align=left><p font-size=13 font-color=D1D1D1>" ..  t1 .. "</p><p line-spacing=3 font-size=12>" .. t2 .. "<br></p><p font-size=8> <br></p></rt>"
	end
end

-- Tabs für die Hilfe, weiß aber nicht, ob das wirklich so funktionieren kann,
-- oder wie ich das eigentlich anwenden muss… Oder was ich falsch mache…
function make_tabs_array(t1, t2)
  return { {
        text = t1,
        tab_picture = "pics/small.png", -- Graphic for the tab button
     },
     {
        text = t2,
        tab_picture = "pics/medium.png",
     }
  }
end

-- RST
-- .. function building_help_general_string(tribename, resourcename, amount, purpose, working_radius, conquer_range, vision_range)
--
--    Creates the string for the general section in building help
--
--    :arg tribename: e.g. "barbarians".
--    :arg resourcename: The resource this building produces
--    :arg working_radius: The owrking radious of the building
--    :arg conquer_range: The conquer range of the building
--    :arg vision_range: The vision range of the building
--    :returns: rt of the formatted text
--
function building_help_general_string(tribename, building_description, resourcename, info, purpose, working_radius, conquer_range)
	if (info) then local info = rt(p(info)) else local info ="" end
	return rt(h2(_"General")) ..
		info ..
		rt(h3(_"Purpose:")) ..
		image_line("tribes/" .. tribename .. "/" .. resourcename  .. "/menu.png", 1, p(purpose)) ..
		text_line(_"Working radius:", working_radius) ..
		text_line(_"Conquer range:", conquer_range) ..
		-- TODO vision range is buggy - returns 0 instead of 4 for lumberjacks_hut, for example
		text_line(_"Vision range:", building_description.vision_range)
end


-- RST
-- .. function building_help_lore_string(tribename, buildingname, flavourtext[, author])
--
--    Displays the building's main image with a flavour text.
--
--    :arg tribename: e.g. "barbarians".
--    :arg buildingname: e.g. "lumberjacks_hut".
--    :arg flavourtext: e.g. "Catches fish in the sea".
--    :arg author: e.g. "Krumta, carpenter of Chat'Karuth". This paramater is optional.
--    :returns: rt of the image with the text
--
function building_help_lore_string(tribename, buildingname, flavourtext, author)
	local result = rt(h2(_"Lore")) ..
		rt("image=tribes/" .. tribename .. "/" .. buildingname  .. "/" .. buildingname .. "_i_00.png", p(flavourtext))
		if author then
			result = result .. rt("text-align=right",p("font-size=10 font-style=italic", author))
		end
	return result
end


-- RST
-- .. function:: building_help_depencencies_ware(tribename, items, ware)
--
--    Formats a chain of ware dependencies for the help window
--
--    :arg tribename: e.g. "barbarians".
--    :arg items: an array with ware and building names,
--                            e.g. {"constructionsite", "trunk"}
--    :arg ware: the ware to use as a title.
--    :returns: an rt string with images describing a chain of ware/building dependencies
--
function building_help_depencencies_ware(tribename, items, ware)
	local ware_descr = wl.Game():get_ware_description(tribename, ware)
	return dependencies(tribename, items, ware_descr.name)		
end


-- RST
-- .. function:: building_help_depencencies_building(tribename, items, building)
--
--    Formats a chain of ware dependencies for the help window
--
--    :arg tribename: e.g. "barbarians".
--    :arg items: an array with ware and building names,
--                            e.g. {"constructionsite", "trunk"}
--    :arg building: the building to use as a title.
--    :returns: an rt string with images describing a chain of ware/building dependencies
--
function building_help_depencencies_building(tribename, items, buildingname)
	local building_descr = wl.Game():get_building_description(tribename,buildingname)
	return dependencies(tribename, items, building_descr.name)		
end



-- RST
-- TODO this function is obsolete
-- .. function building_help_size_string(tribename, buildingname)
--
--    Creates a text_line that describes the building's size in a help text.
--
--    :arg tribename: e.g. "barbarians".
--    :arg buildingname: e.g. "lumberjacks_hut".
--    :returns: "Space required" header followed by size description text and image.
--
function building_help_size_string(tribename, buildingname)

  local building_descr = wl.Game():get_building_description(tribename,buildingname)

  if(building_descr.ismine) then
	return text_line(_"Space required:",_"Mine plot","pics/mine.png")
  elseif(building_descr.isport) then
	return text_line(_"Space required:",_"Port plot","pics/port.png")
  else
	if (building_descr.size == 1) then
 		return text_line(_"Space required:",_"Small plot","pics/small.png")
	elseif (building_descr.size == 2) then
  		return text_line(_"Space required:",_"Medium plot","pics/medium.png")
	elseif (building_descr.size == 3) then
		return text_line(_"Space required:",_"Big plot","pics/big.png")
	else
		return p(_"Space required:" .. _"Unknown")
	end
  end
end


-- RST
--
-- TODO: Causes panic in image_line if resource does not exist
--
-- .. function:: building_help_building_section(tribename, building_description)
--
--    Formats the "Building" section in the building help: Upgrading info, costs and spaqce required
--
--    :arg tribename: e.g. "barbarians".
--    :arg building_description: The building description we get from C++
--    :returns: an rt string describing the building section
--
function building_help_building_section(tribename, building_description)

	local result = rt(h2(_"Building"))

	if(building_description.ismine) then
		result = result .. text_line(_"Space required:",_"Mine plot","pics/mine.png")
	elseif(building_description.isport) then
		result = result .. text_line(_"Space required:",_"Port plot","pics/port.png")
	else
		if (building_description.size == 1) then
			result = result .. text_line(_"Space required:",_"Small plot","pics/small.png")
		elseif (building_description.size == 2) then
			result = result .. text_line(_"Space required:",_"Medium plot","pics/medium.png")
		elseif (building_description.size == 3) then
			result = result .. text_line(_"Space required:",_"Big plot","pics/big.png")
		else
			result = result .. p(_"Space required:" .. _"Unknown")
		end
	end

	if (building_description.enhanced) then
		-- todo get the building this was upgraded from
		result = result .. text_line(_"Upgraded from:", "TODO!!!")

		result = result .. rt(h3(_"Upgrade Cost:"))

		for ware, count in pairs(building_description.enhancement_cost) do
		local ware_descr = wl.Game():get_ware_description(tribename,ware)
			result = result .. 
				image_line("tribes/" .. tribename .. "/" .. ware  .. "/menu.png",
					count, p(count .. " ".. ware_descr.name))
		end
-- TODO this does not work - needs the build cost of the building this was enhanced from
		result = result .. rt(h3(_"Cost Cumulative:"))

		for ware, count in pairs(building_description.build_cost) do
			local ware_descr = wl.Game():get_ware_description(tribename,ware)
			local amount = building_description.build_cost[ware] + building_description.enhancement_cost[ware]
			result = result .. 
				image_line("tribes/" .. tribename .. "/" .. ware  .. "/menu.png",
					amount, p(amount .. " ".. ware_descr.name))
		end

		result = result .. rt(h3(_"Dismantle yields:"))

		for ware, count in pairs(building_description.returned_wares_enhanced) do
			local ware_descr = wl.Game():get_ware_description(tribename,ware)
			result = result .. 
				image_line("tribes/" .. tribename .. "/" .. ware  .. "/menu.png",
					count, p(count .. " ".. ware_descr.name))
		end


	else
		result = result .. rt(h3(_"Build Cost:"))

		for ware, count in pairs(building_description.build_cost) do
			local ware_descr = wl.Game():get_ware_description(tribename,ware)
			result = result .. 
				image_line("tribes/" .. tribename .. "/" .. ware  .. "/menu.png",
					count, p(count .. " ".. ware_descr.name))
		end

		result = result .. rt(h3(_"Dismantle yields:"))

		for ware, count in pairs(building_description.returned_wares) do
			local ware_descr = wl.Game():get_ware_description(tribename,ware)
			result = result .. 
				image_line("tribes/" .. tribename .. "/" .. ware  .. "/menu.png",
					count, p(count .. " ".. ware_descr.name))
		end
	end

	-- TODO get this from C++
	text_line(_"Upgradeable to:","TODO")

	return result
end



-- RST
-- .. function building_help_crew_string(tribename, workername, amount)
--
--    Displays a worker with an image
--
--    :arg tribename: e.g. "barbarians".
--    :arg workername: e.g. "lumberjack".
--    :returns: image_line for the worker
--
function building_help_crew_string(tribename, workername)
	local worker_descr = wl.Game():get_worker_description(tribename,workername)
	return image_line("tribes/" .. tribename .. "/" .. workername  .. "/menu.png", 1, p(worker_descr.name))
	-- todo get localised name from tribe's main conf, and add ngettext!
end

-- RST
-- .. function building_help_tool_string(tribename, toolname)
--
--    Displays a tool with an intro text and image
--
--    :arg tribename: e.g. "barbarians".
--    :arg toolname: e.g. "felling_axe".
--    :returns: text_line for the tool
--
function building_help_tool_string(tribename, toolname, no_of_workers)
	local ware_descr = wl.Game():get_ware_description(tribename,toolname)
	return text_line("Worker uses:", -- todo ngettext("Worker uses:","Workers use:",no_of_workers)
		ware_descr.name, "tribes/" .. tribename .. "/" .. toolname  .. "/menu.png")
	-- todo get localised name from tribe's main conf, and add ngettext!
end


