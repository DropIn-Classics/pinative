# Main image subsystem map

This map is derived only from static inspection of the decoded `ILLUSION.386`
image. Offsets are relative to the start of its flat image. Names marked
"provisional" describe observed behavior without claiming original symbols.

This was found with an earlier scanner (`x86_inventory.py`) that is no longer in the
repository (it is in build/earlier/ on the machine it was removed on);
to be checked again in the hints once the 32-bit stage 1 exists.

For the identified GOG image, the recursive pass starts at entry `0x2A3` and
the 22 valid relocated far-pointer targets. It currently reaches 4,044
instructions occupying 12,018 bytes, with no conflicting instruction starts.
Indirect dispatch and embedded data prevent this from being a complete code
map, but every listed instruction is reached through a traced control-flow
path or a loader-relocated function pointer.

## pMAX interfaces

The game selects pMAX services with `AH` and interrupts `0x92` through `0x94`.
The following meanings are supported directly by call-site behavior. Unknown
services remain unnamed rather than being guessed.

| Interrupt | AH | Observed role | Evidence |
| ---: | ---: | --- | --- |
| `0x92` | `0x04` | Allocate a temporary segment | VGA setup passes size `0x13EA`, receives a selector in `EAX`, fills it, then releases it with service `0x05`. |
| `0x92` | `0x05` | Release a selector/allocation | Audio callbacks pass the selector in `BX`; VGA setup releases its temporary segment the same way. |
| `0x92` | `0x0A` | Allocate named memory for loaded audio code | Two audio callback tables pass a byte count in `EBX` and a diagnostic name in `ESI`, receiving a selector in `AX`. |
| `0x92` | `0x0B` | Query available memory | Entry sums returned registers and requires at least `0x2F0800` bytes. |
| `0x93` | `0x03` | Get a protected-mode interrupt vector | Sound drivers pass the interrupt number in `BL`, then retain returned `ES:EDX` so it can be restored later. |
| `0x93` | `0x04` | Set a protected-mode interrupt vector | Sound drivers pass the interrupt number in `BL` and an ISR or previously saved vector in `ES:EDX`. |
| `0x93` | `0x08` | Create/configure a callable selector into another segment | Audio initialization passes a resource selector in `BX`, descriptor/access value `0x409A` in `DX`, and stores the returned selector beside a zero offset for far calls. |
| `0x93` | `0x0B` | Obtain a selector's linear base | Both audio callback tables pass `BX` and move returned `EAX` into `EBX`. |
| `0x93` | `0x0C` | Return a zero-based flat selector | The main image installs the result in `ES` or `GS` before reading the VGA BIOS at linear `0xC0000`, BIOS data, and pMAX linear allocation offsets. Callback 10 returns the same selector to sound drivers. |
| `0x93` | `0x11` | Return command-line pointer | Entry and the option parser consume the returned `EAX:EDX` pointer. |
| `0x93` | `0x13` | Map a real-mode/DOS transfer buffer | The MSCDEX block uses the returned selector to copy request packets around interrupt `0x2F`, function `0x1510`. |
| `0x93` | `0x15` | Identify the active host/runtime | The option parser compares returned `AX` with `VS`, `RG`, and `GS` signatures. |
| `0x94` | `0x01` | Load a named resource | Callers pass a resource path in `EDX`, a diagnostic label in `ESI`, and retain returned selector `AX`. |
| `0x94` | `0x05` | Release a loaded resource | Used on the same resource state immediately before reloads. |
| `0x94` | `0x06` | Store configuration state | Graphics selection passes the configuration block at `0x9D`. |
| `0x94` | `0x07` | Load configuration state | Startup passes the configuration block at `0x9D`. |
| `0x94` | `0x08` | Open the executable resource library | Startup passes the copied executable path at `0x86C`. |

Services `0x92/0x01`, `0x92/0x02`, `0x92/0x08`, `0x93/0x05`, `0x93/0x07`,
and `0x93/0x12` are reachable but need more register-flow evidence before
assigning semantic names.

## Graphics and display

| Range or entry | Provisional name | Observed behavior |
| ---: | --- | --- |
| `0x49A` | `vga_enter_chooser_mode` | Enters BIOS mode `0x13`, changes CRTC registers, waits across vertical retraces, expands an embedded data block, writes 768 DAC bytes, and copies decoded graphics into video memory. |
| `0x581` | `detect_s3_bios` | Maps the VGA BIOS region and scans its first 256 bytes for the `S3` signature. |
| `0x5BE` | `query_vesa_info` | Calls VBE function `0x4F00`, checks the `VESA` signature, and returns a mode-related value. |
| `0x5FF` | `select_vesa_path` | Tries display modes and validates the selected mode through the routines below. |
| `0x65C` | `validate_bios_display_state` | Uses BIOS function `0x1B` and checks the returned state block. |
| `0x698` | `set_bios_mode_probe` | Sets a candidate mode through interrupt `0x10` after writing a DAC probe color. |
| `0x6B1` | `validate_vga_registers` | Checks sequencer, DAC, miscellaneous-output, and CRTC behavior for the requested path. |
| `0x7230` | `intro_planar_blit` | Programs sequencer/graphics-controller plane selection and copies sparse pixel runs into VGA memory. |

The mode-selection function at `0x753` consumes configuration bytes at `0xA2`
and `0xA3`, tries the BIOS/VESA paths above, and persists any corrected choice
through pMAX configuration service `0x94/0x06`. Their position in the complete
512-byte runtime block is documented in [configuration.md](configuration.md).

## Resource loading

The chooser/intro loader begins around `0x757D`. Repeated call blocks use the
same shape:

1. Put a resource path such as `CHOOSER\\CUBE.RIX`, `INTRO\\PCSKY.FLD`, or
   `INTRO\\MOD.INT` in `EDX`.
2. Put a short diagnostic label in `ESI`.
3. Invoke pMAX resource service `0x94/0x01`.
4. Retain returned selector `AX` in image state.

This makes the `0x75C2` through `0x80D9` region the chooser/intro resource
manifest and loading boundary. Service `0x94/0x05` releases previous resource
state before reloads.

## Digital audio and module playback

Two callback tables have the same 11-entry ABI:

| Table | Initializer | Data-segment state | Use |
| ---: | ---: | ---: | --- |
| `0x618D` | `0x61D9` | `0x635E` | Chooser/intro audio driver |
| `0x98FE` | `0x994A` | `0xA10B` | Table music driver |

Each initializer walks six-byte `{ offset32, selector16 }` far pointers until
an offset sentinel of `0xFFFFFFFF`, replacing every selector with `CS`. The
callbacks wrap pMAX allocation, selector, resource-load, copy, and cleanup
services and return far to dynamically loaded audio code.

The chooser audio path at `0x7075`/`0x7182` loads the configured `Sound Driver`
resource, creates a callable code selector using descriptor/access value
`0x409A`, and calls driver offset zero through the far pointer at `0x8134`.
Observed command values in `EAX` are:

| EAX | Observed call context |
| ---: | --- |
| `0` | Initialize the loaded driver with the callback table at `0x618D`. |
| `1` | Called before the intro loop with `ECX=0x0F`; carry reports failure. |
| `3` | Called during intro cleanup. |
| `4` | Called with `INTRO\\MOD.INT` or `CHOOSER\\MOD.MUS` in `EDX`. |
| `6` | Exposed through a standalone wrapper at `0x7223`. |
| `8` | Called during final cleanup with `BL=0x12`, `CL=0`. |
| `0x0C` | Called after command `4` loads chooser music. |

The driver begins at offset zero and dispatches commands `0x00` through `0x12`.
Its handlers and observed register contracts are mapped in
[audio-driver.md](audio-driver.md). `0x409A` is selector configuration data,
not a driver offset.

## CD audio

The block beginning at `0x35B0D` is a separate MSCDEX interface:

| Entry | Provisional name | Evidence |
| ---: | --- | --- |
| `0x35B0D` | `mscdex_send_request` | Copies a 25-byte request header into mapped DOS memory and invokes interrupt `0x2F` with `AX=0x1510`. |
| `0x35B52` | `mscdex_detect` | Calls interrupt `0x2F` with `AX=0x1500`; saves the first drive number from `CX` when `BX` reports at least one drive. |
| `0x35B7D` onward | `mscdex_requests` | Builds IOCTL and audio request packets, checks request status bits, reads track information, and submits packets through `0x35B0D`. |

This boundary is independent of the dynamically loaded digital-audio driver.
The exact purpose of each request builder still needs packet-level annotation.

## Input and timing observations

During the chooser/intro sequence, IRQ 1 is masked at PIC port `0x21` and input
is polled directly from keyboard data port `0x60` at `0x79A0` and `0x7A36`.
Observed make codes include Escape (`0x01`) and Space (`0x39`). IRQ 1 is
unmasked during cleanup.

The code at `0x32AD9` programs PIT channel 2 through ports `0x42`, `0x43`, and
`0x61`, then converts the counter result using the PIT frequency constant
`0x1234DC`. It is reached through the embedded sound-setup path and serves as a
timing calibration routine, not yet as evidence for the gameplay clock.

## Remaining gaps

- Dynamically confirm the now-identified SDR voice/module parameters and
  interruption behavior.
- Locate the gameplay keyboard/timer state outside the chooser's polling loop.
- Determine whether any of the remaining 505 bytes in the persisted
  configuration block are live state or reserved space.
- Give the slot-15 link to a slot-31-style display state and slot-26's
  eight-byte module-private trailer narrower gameplay names if dynamic traces
  provide evidence. The event-sequence links in slot 14 and slot 16 and all
  four slot-26 BCD values are now structurally mapped in
  [bpc-module.md](bpc-module.md).
