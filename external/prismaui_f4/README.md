# PrismaUI F4 API header provenance

`PrismaUI_F4_API.h` is copied byte-for-byte from the public PrismaUI developer
repository:

- Project: `PRISMA-USER-INTERFACE-FRAMEWORK/Fallout-4-Prisma-UI-Framework`
- Source path: `src/PrismaUI_F4_API.h`
- Source commit: `0daf5e38d3517772c8c28236c8857e5c037fa3bd`
- SHA-256: `0528233f90e5b3e1662284b39a7f5bcc9bd48aecc3e7005ac96bf1cf87da5a9e`
- Runtime provider validated by this project: PrismaUI_F4 `2.1.1`

The plugin requests `IVPrismaUI12` first so it can use the released interface
identifier, then stores and uses only the inherited `IVPrismaUI10` panel
surface. It retains the older sequential V10 identifier as a compatibility
fallback for PrismaUI_F4 `2.1.0.2`.

Only the API header and its upstream `LICENSE.md` are kept here. PrismaUI
binaries, archives, game files, and unrelated third-party assets are not part
of this repository.

An intentional header update requires:

1. review the upstream framework changes;
2. copy the reviewed header byte-for-byte and record its source commit and hash;
3. rebuild the plugin;
4. repeat target-runtime compatibility testing;
5. update this provenance record.
