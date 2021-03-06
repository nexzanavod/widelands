dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   msgctxt = "empire_building",
   name = "empire_marblemine_deep",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = pgettext("empire_building", "Deep Marble Mine"),
   helptext_script = dirname .. "helptexts.lua",
   icon = dirname .. "menu.png",
   size = "mine",

   enhancement_cost = {
      log = 4,
      planks = 2
   },
   return_on_dismantle_on_enhanced = {
      log = 2,
      planks = 1
   },

   animations = {
      idle = {
         pictures = path.list_files(dirname .. "idle_??.png"),
         hotspot = { 49, 61 },
      },
      working = {
         pictures = path.list_files(dirname .. "working_??.png"),
         hotspot = { 49, 61 },
         fps = 10
      },
      empty = {
         pictures = path.list_files(dirname .. "empty_??.png"),
         hotspot = { 49, 61 },
      },
   },

   aihints = {
      mines = "stones"
   },

   working_positions = {
      empire_miner = 1,
      empire_miner_master = 1
   },

   indicate_workarea_overlaps = {
      empire_marblemine = false,
      empire_marblemine_deep = false,
   },

   inputs = {
      { name = "meal", amount = 6 },
      { name = "wine", amount = 6 }
   },
   outputs = {
      "marble",
      "granite"
   },

   programs = {
      work = {
         -- TRANSLATORS: Completed/Skipped/Did not start mining marble because ...
         descname = _"mining marble",
         actions = {
            "return=skipped unless economy needs marble or economy needs granite",
            "consume=meal wine",
            "sleep=40000",
            "call=mine_produce_marble",
            "call=mine_produce_granite",
            "call=mine_produce_marble",
            "call=mine_produce_granite",
            "call=mine_produce_marble",
            "return=no_stats"
         }
      },
      mine_produce_granite = {
         descname = _"mining marble",
         actions = {
            "animate=working 7800",
            "mine=stones 2 100 5 2",
            "produce=granite",
         }
      },
      mine_produce_marble = {
         descname = _"mining marble",
         actions = {
            "animate=working 7800",
            "mine=stones 2 100 5 2",
            "produce=marble",
         }
      },
      encyclopedia = {
         -- just a dummy program to fix encyclopedia
         descname = "encyclopedia",
         actions = {
            "consume=meal wine",
            "produce=marble:3 granite:2",
         }
      },
   },
   out_of_resource_notification = {
      -- Translators: Short for "Out of ..." for a resource
      title = _"No Marble",
      heading = _"Main Marble Vein Exhausted",
      message =
         pgettext("empire_building", "This marble mine’s main vein is exhausted. Expect strongly diminished returns on investment. This mine can’t be enhanced any further, so you should consider dismantling or destroying it."),
   },
}
