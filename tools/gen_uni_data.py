# Generates uni_data.cpp from "ppucd.txt" (ICU preparsed Unicode Character Database).

# Adds a range 'r = (start, end)' to property 'prop'
def add(prop, r):
	(start, end) = r
	for (i, r2) in enumerate(prop):
		(start2, end2) = r2
		if start <= end2+1 and start2 <= end+1:
			prop[i] = (min(start, start2), max(end, end2))
			return
	prop.append(r)

# Removes a range 'r' from property 'prop'
def remove(prop, r):
	(start, end) = r
	for (i, r2) in enumerate(prop):
		(start2, end2) = r2
		if start <= end2 and start2 <= end:
			new_ranges = []
			if start2 < start:
				new_ranges.append((start2, start-1))
			if end < end2:
				new_ranges.append((end+1, end2))
			prop[i:i+1] = new_ranges
			return

def parse_range(s):
	s = s.split("..")
	start = int(s[0], base=16)
	if len(s) == 1:
		end = start
	else:
		end = int(s[1], base=16)
	return (start, end)

space = []
id_start = []
id_continue = []

categories = {}
subcategories = {}

graphic = []

def setProp(props, prop, r):
	if prop not in props:
		props[prop] = []
	for (prop2, ranges) in props.items():
		if prop2 == prop:
			add(ranges, r)
		else:
			remove(ranges, r)

with open("ppucd.txt") as f:
	for l in f:
		if l.startswith("#"): continue
		cols = l.strip().split(";")
		if cols[0] == "cp" or cols[0] == "block":
			r = parse_range(cols[1])
			props = {}
			for c in cols[2:]:
				parts = c.split("=")
				if len(parts) == 1:
					val = c[0] != "-"
					if not val: c = c[1:]
					props[c] = val
				else:
					prop_names = parts[:-1]
					val = parts[-1]
					for prop in prop_names:
						props[prop] = val
			
			if "WSpace" in props and props["WSpace"]:
				add(space, r)
			if "XIDS" in props:
				if props["XIDS"]:
					add(id_start, r)
				else:
					remove(id_start, r)
			if "XIDC" in props:
				if props["XIDC"]:
					add(id_continue, r)
				else:
					remove(id_continue, r)
			if "gc" in props:
				subcategory = props["gc"]
				category = subcategory[0]
				setProp(subcategories, subcategory, r)
				setProp(categories, category, r)
				if category in "LMNPS" or subcategory == "Zs":
					add(graphic, r)
				else:
					remove(graphic, r)

print(graphic)

def flatten(prop):
	for i in range(len(prop)-1):
		assert prop[i][1] + 1 <= prop[i+1][0] - 1
	prop2 = [x for r in prop for x in r]
	for i in range(len(prop2)-1):
		assert prop2[i] <= prop2[i+1]
	return prop2


header = """\
#include <cstdint>
#include <cstddef>

#include "uni_data.hpp"

"""

def write_prop(f, name, fun_name, prop):
	f.write(f"\nstd::size_t UNI_{name}_RANGES = {len(prop)};\n")
	f.write(f"std::uint32_t UNI_{name}[] = {{\n\t")
	f.write(",".join(map(str, flatten(prop))))
	f.write("\n};\n")
	f.write(f"DEFINE_PROP({fun_name}, {name})\n")

with open("uni_data.cpp", "w") as f:
	f.write(header)
	write_prop(f, "SPACE", "isSpace", space)
	write_prop(f, "ID_START", "isIdStart", id_start)
	write_prop(f, "ID_CONTINUE", "isIdContinue", id_continue)
	write_prop(f, "GRAPHIC", "isGraphic", graphic)
	
	# f.write(f"\nenum GeneralCategory {{\n\t")
	# for gc in subcategories:
	# 	f.write(f"{gc}, ")
	# f.write(f"\n}};\n")
