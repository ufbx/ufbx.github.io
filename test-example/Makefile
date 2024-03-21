CC=clang

example: test.c ufbx.c example_graphics.h sokol/sokol_gfx.h sokol/sokol_app.h
	clang -g -lm -lGL -lX11 -lXcursor -lXi -o example test.c ufbx.c

run: example
	./example barbarian.fbx