# Single-file project customization.
#
# `REPLACEMENT_DOMAIN` must not be longer than "nintendo.net" (12 chars),
# because these values are patched in-place.

PROJECT_NAME := Plumbus
PROJECT_MODULE_NAME := plumbus
REPLACEMENT_DOMAIN := quevedo.me

# PEM bundle to embed and inject at runtime.
CA_PEM_SOURCE := data/ca.pem
