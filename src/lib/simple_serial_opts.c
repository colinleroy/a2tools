
static char *opt_tty_path = NULL;
static int opt_tty_speed = B9600;
static int opt_tty_hw_handshake = 1;

static const char *get_cfg_path(void) {
  static char path[255];

#ifdef CONF_FILE_PATH
  return CONF_FILE_PATH;
#endif

  if (getenv("HOME") == NULL) {
    printf("Please export $HOME.\n");
    exit(1);
  }

  snprintf(path, 255, "%s", getenv("HOME"));

  return path;
}

static int tty_speed_from_str(char *tmp) {
  if (!strcmp(tmp, "300"))
    return B300;
  if (!strcmp(tmp, "600"))
    return B600;
  if (!strcmp(tmp, "1200"))
    return B1200;
  if (!strcmp(tmp, "2400"))
    return B2400;
  if (!strcmp(tmp, "4800"))
    return B4800;
  if (!strcmp(tmp, "9600"))
    return B9600;
  if (!strcmp(tmp, "19200"))
    return B19200;
  if (!strcmp(tmp, "57600"))
    return B57600;
  if (!strcmp(tmp, "115200"))
    return B115200;
  printf("Unhandled speed %s.\n", tmp);
  exit(1);
}

static char *tty_speed_to_str(int speed) {
  if (speed == B300)
    return "300";
  if (speed == B600)
    return "600";
  if (speed == B1200)
    return "1200";
  if (speed == B2400)
    return "2400";
  if (speed == B4800)
    return "4800";
  if (speed == B9600)
    return "9600";
  if (speed == B19200)
    return "19200";
  if (speed == B57600)
    return "57600";
  if (speed == B115200)
    return "115200";
  return "???";
}

static int get_bool(char *tmp) {
  return !strcmp(tmp, "1") 
    || !strcasecmp(tmp, "yes")
    || !strcasecmp(tmp, "true")
    || !strcasecmp(tmp, "on");
}

static void simple_serial_write_defaults(void) {
  FILE *fp = NULL;
  fp = fopen(get_cfg_path(), "w");
  if (fp == NULL) {
    printf("Cannot open %s for writing: %s\n", get_cfg_path(),
           strerror(errno));
    exit(1);
  }
  fprintf(fp, "tty: /dev/ttyUSB0\n"
              "baudrate: 9600\n"
              "hw_handshake: on\n"
              "\n"
              "#Alternatively, you can export environment vars:\n"
              "A2_TTY, A2_TTY_SPEED, A2_TTY_HW_HANDSHAKE\n");
  fclose(fp);
  printf("A default configuration file has been generated to %s.\n"
         "Please review it and try again.\n", get_cfg_path());
  exit(1);
}

static int simple_serial_read_opts(void) {
  FILE *fp = NULL;
  char buf[255];
  fp = fopen(get_cfg_path(), "r");
  if (fp == NULL) {
    simple_serial_write_defaults();
    /* We'll be exit()ed there */
  }
  while (fgets(buf, 255, fp)) {
    if(!strncmp(buf, "tty:", 4)) {
      free(opt_tty_path);
      opt_tty_path = trim(buf + 4);
    }

    if (!strncmp(buf,"baudrate:", 9)) {
      char *tmp = trim(buf + 9);
      opt_tty_speed = tty_speed_from_str(tmp);
      free(tmp);
    }

    if (!strncmp(buf,"hw_handshake:", 13)) {
      char *tmp = trim(buf + 13);
      opt_tty_hw_handshake = get_bool(tmp);
      free(tmp);
    }
  }
  fclose(fp);
  
  /* Env vars take precedence */
  if (getenv("A2_TTY")) {
    free(opt_tty_path);
    opt_tty_path = strdup(getenv("A2_TTY"));
  }

  if (getenv("A2_TTY_SPEED")) {
    opt_tty_speed = tty_speed_from_str(getenv("A2_TTY_SPEED"));
  }

  if (getenv("A2_TTY_HW_HANDSHAKE")) {
    opt_tty_hw_handshake = get_bool(getenv("A2_TTY_HW_HANDSHAKE"));
  }
  return 0;
}
