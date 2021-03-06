dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = path.list_files(dirname .. "idle_??.png"),
      hotspot = { 8, 14 },
      fps = 20,
   },
}

add_directional_animation(animations, "walk", dirname, "walk", {11, 21}, 20)


world:new_critter_type{
   name = "lynx",
   descname = _ "Lynx",
   editor_category = "critters_carnivores",
   attributes = { "eatable" },
   programs = {
      remove = { "remove" },
   },
   animations = animations,
}
