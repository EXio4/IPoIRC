#ifndef IPOIRC_CONFIG_H
#define IPOIRC_CONFIG_H

#define FORMAT  "@{netid}: {message}"

#define FORMAT_FINAL "@{netid}::{message}"

#define REGEX   "^@([a-zA-Z0-9]+): (.+)"

#define REGEX_FINAL "^@([a-zA-Z0-9]+)::(.+)$"

// internals

#define MTU 1472

#endif
