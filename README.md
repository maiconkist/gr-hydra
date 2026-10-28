# gr-hydra

## PUBLICATIONS

The following papers describe HyDRA and its extensions:

- **[HyDRA: A Hypervisor for Software Defined Radios to Enable Radio Virtualization in Mobile Networks](https://doi.org/10.1109/INFCOMW.2017.8116510)**.
  Maicon Kist, Juergen Rochol, Luiz A. DaSilva, and Cristiano Bonato Both.
  *IEEE INFOCOM Workshops*, 2017, pp. 960-961.
  [Download PDF](https://luizdasilva.wordpress.com/wp-content/uploads/2011/02/kist-demoinfocom17.pdf).

- **[SDR Virtualization in Future Mobile Networks: Enabling Multi-Programmable Air-Interfaces](https://doi.org/10.1109/ICC.2018.8422643)**.
  Maicon Kist, Juergen Rochol, Luiz A. DaSilva, and Cristiano Bonato Both.
  *IEEE International Conference on Communications (ICC)*, 2018, pp. 1-6.
  [Download PDF](https://luizdasilva.wordpress.com/wp-content/uploads/2018/06/1570411395.pdf).

- **[Towards Enabling RAN as a Service - The Extensible Virtualisation Layer](https://doi.org/10.1109/ICC.2019.8761679)**.
  Joao F. Santos, Maicon Kist, Jonathan van de Belt, Juergen Rochol, and
  Luiz A. DaSilva.
  *IEEE International Conference on Communications (ICC)*, 2019, pp. 1-6.
  [Download PDF](https://vtechworks.lib.vt.edu/server/api/core/bitstreams/537e2017-b5f9-4f1a-a6c5-2750eb271713/content).

- **[Virtual Radios, Real Services: Enabling RANaaS Through Radio Virtualisation](https://doi.org/10.1109/TNSM.2020.3009863)**.
  Joao F. Santos, Maicon Kist, Juergen Rochol, and Luiz A. DaSilva.
  *IEEE Transactions on Network and Service Management*, vol. 17, no. 4,
  pp. 2610-2619, 2020.
  [Download PDF](https://orca-project.eu/wp-content/uploads/sites/4/2020/07/santos_tnsm_2020.pdf).

- **[AIRTIME: End-to-End Virtualization Layer for RAN-as-a-Service in Future Multi-Service Mobile Networks](https://doi.org/10.1109/TMC.2020.3046535)**.
  Maicon Kist, Joao F. Santos, Diarmuid Collins, Juergen Rochol,
  Luiz A. DaSilva, and Cristiano Bonato Both.
  *IEEE Transactions on Mobile Computing*, vol. 21, no. 8,
  pp. 2701-2717, 2022.
  [Download PDF](https://vtechworks.lib.vt.edu/bitstream/handle/10919/107563/kist-tmc-2022.pdf).

[![Linux Docker build](https://github.com/maiconkist/gr-hydra/actions/workflows/docker-build.yml/badge.svg?branch=bleeding&event=push)](https://github.com/maiconkist/gr-hydra/actions/workflows/docker-build.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

## GETTING STARTED

### REQUIRED LIBS

The full build targets Ubuntu 24.04, GNU Radio 3.10, and Python 3. CMake 3.16
or later is required for the GNU Radio module and QA.

```sh
sudo apt update
sudo apt install --no-install-recommends \
  build-essential ca-certificates cmake pkg-config \
  libboost-filesystem-dev libboost-log-dev libboost-program-options-dev \
  libboost-system-dev libfftw3-dev libuhd-dev uhd-host libopencv-dev cppzmq-dev \
  gnuradio gnuradio-dev pybind11-dev python3-dev \
  python3-lxml python3-yaml python3-zmq
```

## INSTALLATION

### LINUX DOCKER BUILD

```sh
docker build --pull -t gr-hydra .
docker run --rm gr-hydra
```

The Ubuntu 24.04 image installs the dependencies, builds the core libraries and
applications, installs the GNU Radio C++/Python bindings and GRC definitions, and
runs the standalone QA suites. Running the image repeats QA as a non-root user.
Tests do not require USRP hardware.

Build parallelism defaults to two jobs; override it with
`--build-arg BUILD_JOBS=4` if needed. Sources and build outputs are under
`/opt/hydra`, with the standalone applications in `/opt/hydra/build/app`.
The `Linux Docker build` GitHub Actions workflow builds and runs this image on
an Ubuntu 24.04 runner.

#### STANDARD VERSION - NO GNURADIO SUPPORT

Clone the repository and enter in the created folder (default: gr-hydra). Execute the following commands:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel 2
sudo cmake --install build
sudo ldconfig
```

#### WITH GNURADIO SUPPORT

First install the STANDARD VERSION. From the repository root:

```sh
cmake -S grc_blocks -B grc_blocks/build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build grc_blocks/build --parallel 2
sudo cmake --install grc_blocks/build
sudo ldconfig
```

The module uses Python 3/pybind11 and YAML GRC definitions. The slice manager,
the five `ansible_*.py` applications, and the legacy build/documentation helpers
also use Python 3. The applications and their paired `.grc` sources have been
migrated to GNU Radio 3.10, preserving the existing command-line options and
radio settings. Running the radio applications still requires their configured
USRP devices, TAP interfaces, and network peers.

## TESTING (LINUX)

QA is a standalone CMake project under `tests/`. It uses Boost.Test and CTest
and links against the separately built core library. The core project does not
include QA sources or test targets.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --target hydra
cmake -S tests -B tests/build \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DHYDRA_LIBRARY="$PWD/build/lib/libhydra.so"
cmake --build tests/build
(cd tests/build && ctest --output-on-failure)
```

The QA project requires CMake 3.16 or later. Use matching compiler and ABI
settings for the core and QA builds, including sanitizer or debug-iterator flags.

After installing the GNU Radio module, run the separate Python QA:

```sh
python3 -m unittest discover -s tests -p 'qa_*.py' -v
```

These tests check all repository Python files for Python 3 syntax, exercise the
native control-client bindings over local TCP/ZeroMQ and XML-RPC, and cover the
legacy template/documentation helpers. They also regenerate the five example
flowgraphs, check their command-line entrypoints, and construct them with real
GNU Radio DSP blocks. Hardware, HyDRA-server, and TAP endpoints are replaced by
test doubles; Qt runs headlessly. No physical radio or privileged TAP setup is
required.

## RUNNING THE EXAMPLES

### NOMENCLATURE

For all examples we:

- We associate a terminal ID to a all terminals open in the form **T_n**, where **n** is the terminal number. Wherewer needed we are going to refer to a specific terminal by specifying its ID.

- refer to **USRP_n** as a pair of **PC+USRP**, where **n** indentifies one of the pair. Ex: USRP_1 is what you consider your 1st PC+USRP, while **USRP_2** is your second. Whenever we refer to **USRP_1** you execute the instructions in that machine. 


### EXAMPLE 1
#### HARDWARE REQUIREMENTS: 2x [USRP] -- IDEAL: 3x [USRP]


1. Open a terminal in USRP_1 (**T1**). Start the Hydra-Server flowgraph:
```
python grc_blocks/app/hydra_gr_server_example.py
```
2. Open a terminal in USRP_1 (**T2**). Start the client that request 2 end-to-end TX/RX slices:
```
python grc_blocks/app/hydra_gr_client_2tx_2rx_pdu_gen.py
```
3. Open a terminal in USRP_2 (**T3**). Start the flowgraph that receives the messages (PDU) transmitted in slice 1:
```
python grc_blocks/app/usrp_vr1_pdu_gen.grc
```
4. In **T3** you should see the log of messages being received from USRP_1: 
```
LOG
LOG
LOG
LOG
```
5. In **T2** you should see the log of messages being received from USRP_2:
```
LOG
LOG
LOG
LOG
```
6. If you have only 2 USRP: stop the flowgraph in **T3**. Otherwise go to next step.
7.  Open a terminal (**T4**) in USRP_3 (or USRP_2 if you have only 2 USRPs).  Start the flowgraph that receives the messages (PDU) transmitted in slice 2:
```
python grc_blocks/app/usrp_vr2_pdu_gen.grc
```
8. In **T2** you should see the log of messages being received from **USRP_3** (or **USRP_2**):
```
LOG
LOG
LOG
LOG
```
9. In **T4** you should see the log of messages being received from USRP_1:
```
LOG
LOG
LOG
LOG
```


### EXAMPLE 2
#### HARDWARE REQUIREMENTS: 


## LICENSE

## ACKNOWLEDGEMENTS

## TROUBLESHOOTING

* Cmake cannot find FFTW ? 

   Put the file in https://github.com/jedbrown/cmake-modules/blob/master/FindFFTW.cmake in the folder /usr/share/cmake-2.8/Modules/

## BUGS AND IMPROVEMENT BACKLOG

The following items come from a review of the current implementation. This is
not an exhaustive bug inventory. Optimization ideas are candidates for profiling,
not measured performance improvements.

### Bug status

Resolved items are marked **Fixed**; unmarked entries remain open.

- **Fixed: invalid iterators when releasing spectrum.**
  `rf_front_end::delete_chunks` now checks both list boundaries before
  inspecting neighbors. The `hydra_resource_release` regression cases cover
  full-band, boundary, interior, and adjacent allocations, including preservation
  of occupied neighbors and coalescing of free chunks.
  Source: `lib/hydra_resource.cc`.

- **Fixed: repeated resource release can crash the server.**
  `Hypervisor::detach_virtual_radio` now returns `false` for a missing ID instead
  of dereferencing a null radio. The `hydra_repeated_release` regression covers
  unknown IDs and detaching the same radio more than once.
  Sources: `lib/hydra_hypervisor.cc`, `lib/hydra_core.cc`,
  `lib/hydra_client.cc`, `grc_blocks/lib/hydra_gr_client_*_impl.cc`.

- **Retuning can report success after losing the original allocation.**
  `rf_front_end::check_chunk` checks frequency bounds but not whether a chunk is
  occupied. `HydraCore` then frees the old allocation and ignores a failed
  replacement reservation; `Hypervisor::notify` also reports success when
  mapping fails. Reservation and DSP-map updates need a consistent commit or
  rollback.
  Sources: `lib/hydra_resource.cc`, `lib/hydra_core.cc`,
  `lib/hydra_hypervisor.cc`.

- **Bandwidth changes leave incompatible DSP state.** Updating a radio's
  bandwidth changes its mapping and FFT length, but does not recreate its FFT
  plans, framing configuration, or buffer capacities. Increasing bandwidth can
  make processing access beyond the original sample windows or FFT buffers.
  Sources: `lib/hydra_virtual_radio.cc`, `lib/hydra_hypervisor.cc`,
  `include/hydra/hydra_resampler.hpp`.

- **Malformed or empty control JSON is not handled safely.** The server
  continues to `root.front()` after a JSON parse failure, and an empty object
  also has no first entry. Invalid requests should receive an error without
  entering command dispatch.
  Source: `lib/hydra_server.cc`.

- **Backend receive results are incomplete.** The default UHD receive path,
  image backend, and file backend can reach the end of a non-void function
  without returning a sample count. `Hypervisor::rx_run` also treats the result
  as a boolean and forwards a full FFT window regardless of the actual count,
  potentially processing stale samples after short reads.
  Sources: `lib/hydra_uhd_interface.cc`, `lib/hydra_hypervisor.cc`.

- **Fixed: unsafe single-direction state and cleanup.** Core and hypervisor
  direction flags start disabled. Freeing an unconfigured direction is a
  successful no-op rather than a process exit. Regressions cover RX-only and
  TX-only resource managers, plus cleanup of an unconfigured core.
  Sources: `lib/hydra_core.cc`, `lib/hydra_hypervisor.cc`,
  `lib/hydra_resource.cc`.

- **Worker state is not consistently synchronized.** Stop flags are ordinary
  booleans shared across threads, and radio-vector and mapping accesses do not
  consistently use the same synchronization. Allocation, retuning, streaming,
  and shutdown can therefore race.
  Sources: `include/hydra/hydra_hypervisor.h`,
  `include/hydra/hydra_resampler.hpp`, `lib/hydra_hypervisor.cc`,
  `lib/hydra_virtual_radio.cc`, `lib/hydra_socket.cc`.

- **FFT and worker lifetimes lack complete cleanup.** `fft_complex` allocates
  FFTW buffers and a plan but has no destructor to release them. Hypervisor
  shutdown joins the front-end threads without stopping every virtual radio's
  socket and framing threads. Resource ownership should guarantee orderly
  stop, join, and release.
  Sources: `include/hydra/hydra_fft.h`, `lib/hydra_fft.cc`,
  `include/hydra/hydra_hypervisor.h`, `include/hydra/hydra_virtual_radio.h`.

- **Allocation responses have inconsistent success semantics.** TX response
  parsing returns success (`0`) on malformed JSON, leaving endpoint fields
  unset. Reconfiguring an existing radio returns `1`, which the server publishes
  as its data port instead of returning the existing port.
  Sources: `lib/hydra_client.cc`, `lib/hydra_core.cc`,
  `lib/hydra_server.cc`.

- **Sub-bin bandwidth requests can produce zero-length FFTs.** A positive
  bandwidth smaller than one front-end FFT bin truncates to zero samples.
  Validate the requested bandwidth and resulting FFT length before allocating
  a radio or starting workers.
  Sources: `lib/hydra_virtual_radio.cc`, `lib/hydra_hypervisor.cc`,
  `lib/hydra_fft.cc`.

- **Fixed: the USRP backend ignored the requested RX gain.**
  `device_uhd::set_rx_config` forwards the supplied normalized gain to UHD
  instead of forcing `0.0`, preserving the existing range validation.
  Source: `lib/hydra_uhd_interface.cc`.

### Possible optimizations

- **Replace short polling loops with explicit wakeups.** Profile blocking
  queues or condition-variable notifications instead of repeated one-microsecond
  sleeps in socket and framing workers. Measure CPU use and latency together.
  Sources: `include/hydra/hydra_resampler.hpp`, `lib/hydra_socket.cc`.

- **Reduce hot-path copies and allocations.** Reuse IQ windows and message
  storage, and consider moving buffers where ownership allows it. Buffer reads,
  per-radio FFT input, wideband output, and ZeroMQ messages currently involve
  repeated copies.
  Sources: `include/hydra/hydra_buffer.hpp`, `lib/hydra_virtual_radio.cc`,
  `lib/hydra_hypervisor.cc`, `lib/hydra_socket.cc`.

- **Reduce FFT memory and setup overhead.** Allocate using the single-precision
  complex size rather than `sizeof(fftw_complex)` for float IQ buffers. After
  fixing ownership, investigate FFTW wisdom or reusable planning information
  for repeated radio creation instead of repeating `FFTW_MEASURE` planning.
  Source: `lib/hydra_fft.cc`.

- **Remove or reconnect the idle RX framing stage.** `demap_iq_samples` writes
  directly to the RX resampler's output buffer, while its worker waits on the
  unused `rx_windows` input. A direct output buffer would avoid an unnecessary
  polling thread unless a real conversion stage is introduced.
  Sources: `lib/hydra_virtual_radio.cc`,
  `include/hydra/hydra_resampler.hpp`.

- **Reduce coarse-lock contention after fixing races.** Both streaming paths
  hold the radio-list mutex while processing individual radios. Profile stable
  snapshots or per-radio ownership that permit DSP work outside that global
  critical section.
  Source: `lib/hydra_hypervisor.cc`.

- **Bound and pace emulated streaming.** The loopback backend queues windows
  without a capacity or sample-rate limit. Evaluate bounded queues,
  backpressure, and pacing so emulation does not consume all available CPU or
  accumulate unbounded data.
  Source: `lib/hydra_uhd_interface.cc`.

### Other improvements

- **Expand regression coverage.** Standalone QA and Linux Docker CI now cover
  release boundaries, repeated release, unconfigured directions, Python
  control-client round trips, and GRC definition loading. Add conflicting
  reservations, malformed messages, bandwidth changes, FFT round trips, short
  backend reads, and a headless IQ loopback integration scenario.
  Sources: `tests/`, `.github/workflows/docker-build.yml`.

- **Complete core build metadata.** Both builds now use C++17, and the GNU Radio
  module links the standalone core instead of recompiling it. Finish
  target-based dependency declarations and package exports for the core.
  Sources: `CMakeLists.txt`, `lib/CMakeLists.txt`,
  `grc_blocks/CMakeLists.txt`, `grc_blocks/lib/CMakeLists.txt`.

- **Supported GNU Radio and Python build baseline: implemented.** The
  integration targets GNU Radio 3.10, Python 3, pybind11, and YAML block
  definitions. Legacy generated examples still need updating as described below.
  Sources: `grc_blocks/CMakeLists.txt`, `grc_blocks/python/bindings.cc`,
  `grc_blocks/grc/`, `grc_blocks/app/`.

- **Document the actual protocol and radio identity model.** Control uses
  `xvl_*` JSON messages over ZeroMQ REQ/REP; active IQ streaming uses raw
  `complex<float>` samples over ZeroMQ PUSH/PULL on TCP. The `udp_port` names are
  legacy, while UDP ports 5001/5002 serve discovery. Document endpoint ownership,
  shared TX/RX radio IDs, and the reserved free-space markers: chunk ID `0` and
  FFT-bin ID `-1`.
  Sources: `lib/hydra_server.cc`, `lib/hydra_socket.cc`,
  `include/hydra/types.h`, `lib/hydra_resource.cc`,
  `lib/hydra_hypervisor.cc`.

- **Make sample-rate and FFT-bin constraints explicit.** `resampler` currently
  only packs or unpacks sample windows; it does not convert sample rates.
  Account for integer bin-width quantization and hardware-coerced rates, expose
  the actual configuration, and either reject unsupported rates or implement
  the required conversion.
  Sources: `include/hydra/hydra_resampler.hpp`,
  `lib/hydra_hypervisor.cc`, `lib/hydra_uhd_interface.cc`.

- **Refresh runnable examples.** Several example paths above do not exist in
  this checkout. Replace them with maintained examples and document the
  standalone server's backend options. Update `iq_generator` to discover the
  server and use the active TCP/float-IQ transport rather than its older
  UDP/double-IQ implementation.
  Sources: `grc_blocks/app/`, `app/server.cc`, `app/iq_generator.cc`.

- **Expose overload and lifecycle failures.** Report rejected buffer writes,
  ZeroMQ send timeouts, backend underruns/overruns, and queue depth. The existing
  statistics path is not wired into the main runtime. Replace library-level
  process exits with explicit error results and define bounded shutdown behavior
  when the server or a client disappears.
  Sources: `include/hydra/hydra_buffer.hpp`, `lib/hydra_socket.cc`,
  `lib/hydra_stats.cc`, `lib/hydra_main.cc`, `lib/hydra_client.cc`.
