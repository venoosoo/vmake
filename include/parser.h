#pragma once

struct Target** parse(char* config_path);

void check_source_dirs(struct Target* target);