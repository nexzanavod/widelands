dirname = path.dirname (__file__)

tribes:new_productionsite_type {
   msgctxt = "frisians_building",
   name = "frisians_charcoal_burners_house",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = pgettext ("frisians_building", "Charcoal Burner's House"),
   helptext_script = dirname .. "helptexts.lua",
   icon = dirname .. "menu.png",
   size = "small",

   buildcost = {
      brick = 2,
      granite = 1,
      log = 1,
      reed = 1
   },
   return_on_dismantle = {
      brick = 2,
      log = 1,
      reed = 1
   },

   spritesheets = {
      idle = {
         directory = dirname,
         basename = "idle",
         hotspot = {43, 74},
         frames = 10,
         columns = 5,
         rows = 2,
         fps = 10
      },
   },

   animations = {
      unoccupied = {
         directory = dirname,
         basename = "unoccupied",
         hotspot = {43, 56}
      }
   },

   aihints = {
      prohibited_till = 760,
      requires_supporters = true,
   },

   working_positions = {
      frisians_charcoal_burner = 1
   },

   inputs = {
      { name = "log", amount = 6 },
   },

   outputs = {
      "coal"
   },

   indicate_workarea_overlaps = {
      frisians_aqua_farm = false,
      frisians_charcoal_burners_house = false,
      frisians_clay_pit = true,
   },

   programs = {
      work = {
         -- TRANSLATORS: Completed/Skipped/Did not start working because ...
         descname = _"working",
         actions = {
            "call=erect_stack",
            "call=collect_coal",
            "return=no_stats",
         },
      },
      erect_stack = {
         -- TRANSLATORS: Completed/Skipped/Did not start making a charcoal stack because ...
         descname = _"making a charcoal stack",
         actions = {
            "return=skipped unless economy needs coal",
            "return=failed unless site has log:3",
            "callworker=make_stack",
            "consume=log:3",
            "sleep=15000",
         },
      },
      collect_coal = {
         -- TRANSLATORS: Completed/Skipped/Did not start collecting coal because ...
         descname = _"collecting coal",
         actions = {
            "return=skipped unless economy needs coal",
            "sleep=15000",
            "callworker=collect_coal",
         },
      },
   },

   out_of_resource_notification = {
      -- Translators: Short for "Out of ..." for a resource
      title = _"No Ponds",
      heading = _"Out of Clay Ponds",
      message = pgettext ("frisians_building", "The charcoal burner working at this charcoal burner's house can’t find any clay ponds in his work area. Please make sure there is a working clay pit nearby and the charcoal kiln is supplied with all needed wares, or consider dismantling or destroying this building."),
      productivity_threshold = 12
   },
}
