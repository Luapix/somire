# Generates uni_data.cpp from "ppucd.txt" (ICU preparsed Unicode Character Database).

# Adds a range 'r = (start, end)' to property 'prop'
def add(prop, r):
	if len(prop) == 0 or prop[-1][1] != r[0] - 1:
		prop.append(r)
	else:
		prop[-1] = (prop[-1][0], r[1])

# Removes a range 'r' from property 'prop'
def remove(prop, r):
	# There are currently no ranges with block-overriding negative properties:
	assert r[0] == r[1]
	cp = r[0]
	i = 0
	while i < len(prop):
		(start, end) = prop[i]
		if start <= cp <= end:
			new_ranges = []
			if start < cp:
				new_ranges.append((start, cp-1))
			if cp < end:
				new_ranges.append((cp+1, end))
			prop[i:i+1] = new_ranges
			return
		i += 1

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

with open("ppucd.txt") as f:
	for l in f:
		if l.startswith("#"): continue
		l = l.strip().split(";")
		if l[0] == "cp" or l[0] == "block":
			#print(l)
			r = parse_range(l[1])
			if "WSpace" in l:
				#print(r)
				add(space, r)
			if "XIDS" in l:
				add(id_start, r)
			elif "-XIDS" in l:
				remove(id_start, r)
			if "XIDC" in l:
				add(id_continue, r)
			elif "-XIDC" in l:
				remove(id_continue, r)


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
	write_prop(f, "SPACE", "is_space", space)
	write_prop(f, "ID_START", "is_id_start", id_start)
	write_prop(f, "ID_CONTINUE", "is_id_continue", id_continue)
