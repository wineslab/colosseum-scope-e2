# SCOPE E2 Interface

This repository provides an implementation of the DU-side E2 termination to connect to the O-RAN near-real-time RIC, e.g., the one provided [here](https://github.com/wineslab/colosseum-near-rt-ric). This code has beed adapted from the O-RAN Software Community du-l2 repository and extended to connect to the base station provided as part of the [SCOPE](https://github.com/wineslab/colosseum-scope) framework.

If you use this software, please reference the following paper:

XXX

This work was partially supported by the U.S. National Science Foundation under Grants CNS-1923789 and NSF CNS-1925601, and the U.S. Office of Naval Research under Grant N00014-20-1-2132.

## Quick start

From the `root` directory of the cloned repository:
- From the [build_odu.sh](build_odu.sh) file: configure the near-real-time RIC IP address (`RIC_HOST`), port (`RIC_PORT`), and interface to reach the RIC from this host (`INTERFACE_TO_RIC`)
- Build: `./build_odu.sh` (a clean build may be necessary if modifying the parameters from the build file: `./build_odu.sh clean`)
- After the near-real-time RIC has started (see [here](https://github.com/wineslab/colosseum-near-rt-ric)): `./run_odu.sh`

## Integration with SCOPE

The code in this repository is designed to run on the Colosseum testbed as part of the SCOPE framework (and cloned in the SCOPE [radio_code](https://github.com/wineslab/colosseum-scope/tree/main/radio_code) directory).
If not, it may be necessary to slightly adapt the configuration parameters in [srs_connector.h](src/du_app/srs_connector.h) (e.g., `CONFIG_PATH`, `SCHEDULING_FILENAME`, and `SLICING_BASE_FILENAME`).

It can interact with the files in the SCOPE [scope_config](https://github.com/wineslab/colosseum-scope/tree/main/radio_code/scope_config) directory (e.g., to read metrics or implement control actions received from the xApps running on the near-real-time RIC).

Once connected to the near-real-time RIC, this implementation is capable of:
- Transmitting periodic RAN metrics to subscribed xApps with a periodicity defined by the subscribing xApp through RIC Indication Messages
- Receive control actions from the xApp through RIC Control Messages. The supported control actions allow to modify scheduling and slicing policies of the SCOPE base station. Messages should be in the following format: `<comma-separated slicing policy for each slice>\n<comma-separated number of RBG for each slice`.<sup>[1](#footnote1)</sup>
For instance, the control message `1,0,0\n5,10,3` implements the slicing policiy `1` for the first slice, and `0` for the second and third slices. The control message also sets `5` RBGs for the first slice, `10` for the second, and `3` for the third. (See [this](https://github.com/wineslab/colosseum-scope#radio_code) for more details on the meaning of these values.)

## Build file

Some configurations can be tuned directly from the [build_odu.sh](build_odu.sh) file:
- `RIC_HOST`: IP address of the near-real-time RIC to connect to
- `RIC_PORT`: port the near-real-time RIC is listening to
- `INTERFACE_TO_RIC`: interface used by the host to reach the near-real-time RIC
- `DEBUG`: if enabled (set to `1`), the E2 termination will transmit test metrics to the subscribed xApp. If disabled (set to `0`), run-time metrics corresponding to the served users will be transmitted. By default, these metrics are taken from the SCOPE CSV-formatted metrics [directory](https://github.com/wineslab/colosseum-scope/tree/main/radio_code/scope_config/metrics/csv)


---
<a id="footnote1">1</a>Omit the second line of the control message to only transmit a scheduling policy control, e.g., `1,0,0`. Start the control message with a newline character (`\n`) to only transmit a slicing allocation control, e.g., `\n5,10,3`
