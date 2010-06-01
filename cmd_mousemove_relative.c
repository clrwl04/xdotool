#include "xdo_cmd.h"
#include <math.h>

int cmd_mousemove_relative(int argc, char **args) {
  int x, y;
  int ret = 0;
  char *cmd = *args;
  int polar_coordinates = 0;
  int clear_modifiers = 0;
  int opsync = 0;
  int origin_x = -1, origin_y = -1;

  xdo_active_mods_t *active_mods = NULL;
  int c;
  typedef enum {
    opt_unused, opt_help, opt_sync, opt_clearmodifiers, opt_polar
  } optlist_t;
  static struct option longopts[] = {
    { "help", no_argument, NULL, opt_help },
    { "sync", no_argument, NULL, opt_sync },
    { "polar", no_argument, NULL, opt_polar },
    { "clearmodifiers", no_argument, NULL, opt_clearmodifiers },
    { 0, 0, 0, 0 },
  };
  static const char *usage =
      "Usage: %s [options] <x> <y>\n"
      "-c, --clearmodifiers      - reset active modifiers (alt, etc) while typing\n"
      "-p, --polar               - Use polar coordinates. X as an angle, Y as distance\n"
      "--sync                    - only exit once the mouse has moved\n"
      "\n"
      "Using polar coordinate mode makes 'x' the angle (in degrees) and\n"
      "'y' the distance.\n"
      "\n"
      "If you want to use negative numbers for a coordinate,\n"
      "you'll need to invoke it this way (with the '--'): %s -- -20 -15\n"
      "otherwise, normal usage looks like this: %s 100 140\n";
  int option_index;

  while ((c = getopt_long_only(argc, args, "cph", longopts, &option_index)) != -1) {
    switch (c) {
      case 'h':
      case opt_help:
        printf(usage, cmd, cmd, cmd);
        return EXIT_SUCCESS;
        break;
      case 'p':
      case opt_polar:
        polar_coordinates = 1;
        break;
      case opt_sync:
        opsync = 1;
        break;
      case 'c':
      case opt_clearmodifiers:
        clear_modifiers = 1;
        break;
      default:
        fprintf(stderr, usage, cmd, cmd, cmd);
        return EXIT_FAILURE;
    }
  }

  argc -= optind;
  args += optind;

  if (argc != 2) {
    fprintf(stderr, usage, cmd, cmd, cmd);
    fprintf(stderr, "You specified the wrong number of args.\n");
    return 1;
  }

  x = atoi(args[0]);
  y = atoi(args[1]);

  /* Quit early if we don't have to move. */
  if (x == 0 && y == 0) {
    return EXIT_SUCCESS;
  }

  if (polar_coordinates) {
    /* The original request for polar support was that '0' degrees is up
     * and that rotation was clockwise, so 0 is up, 90 right, 180 down, 270
     * left. This conversion can be done with (360 - degrees) + 90 */
    double radians = ((360 - x) + 90) * (M_PI / 180);
    double distance = y;
    x = (cos(radians) * distance);

    /* Negative sin, since screen Y coordinates are descending, where cartesian
     * is ascending */
    y = (-sin(radians) * distance);
  }
 
  if (clear_modifiers) {
    active_mods = xdo_get_active_modifiers(xdo);
    xdo_clear_active_modifiers(xdo, CURRENTWINDOW, active_mods);
  }

  if (opsync) {
    xdo_mouselocation(xdo, &origin_x, &origin_y, NULL);
  }

  ret = xdo_mousemove_relative(xdo, x, y);

  if (ret) {
    fprintf(stderr, "xdo_mousemove_relative reported an error\n");
  } else {
    if (opsync) {
      /* Wait until the mouse moves away from its current position */
      xdo_mouse_wait_for_move_from(xdo, origin_x, origin_y);
    }
  }

  if (clear_modifiers) {
    xdo_set_active_modifiers(xdo, CURRENTWINDOW, active_mods);
    xdo_free_active_modifiers(active_mods);
  }

  return ret;
}

