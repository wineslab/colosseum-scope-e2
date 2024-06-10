# SCOPE E2 Interface

This repository is part of the [OpenRAN Gym](https://openrangym.com) project. It provides an implementation of the RAN-side E2 termination to connect to the O-RAN near-real-time RIC, e.g., the one provided [here](https://github.com/wineslab/colosseum-near-rt-ric). This code has beed adapted from the O-RAN Software Community [o-du-l2](https://github.com/o-ran-sc/o-du-l2) repository and extended to connect to the base station provided as part of the [SCOPE](https://github.com/wineslab/colosseum-scope) framework.

If you use this software, please reference the following papers:

> L. Bonati, M. Polese, S. D'Oro, S. Basagni, T. Melodia, "OpenRAN Gym: An Open Toolbox for Data Collection and Experimentation with AI in O-RAN," Proc. of IEEE WCNC Workshop on Open RAN Architecture for 5G Evolution and 6G, Austin, TX, USA, April 2022. [bibtex](https://ece.northeastern.edu/wineslab/wines_bibtex/bonati2022openrangym.txt) [pdf](https://arxiv.org/pdf/2202.10318.pdf)

> L. Bonati, S. D'Oro, S. Basagni, and T. Melodia, "SCOPE: An Open and Softwarized Prototyping Platform for NextG Systems," in Proceedings of ACM MobiSys, June 2021 [bibtex](https://ece.northeastern.edu/wineslab/wines_bibtex/bonati2021scope.txt) [pdf](https://ece.northeastern.edu/wineslab/papers/bonati2021scope.pdf)

This work was partially supported by the U.S. National Science Foundation under Grants CNS-1923789 and NSF CNS-1925601, and the U.S. Office of Naval Research under Grant N00014-20-1-2132.

## Quick start

We provide a Colosseum LXC container that contains this repository, the SCOPE framework, and their prerequisites. The container `scope-with-e2` can be found among the images available for Colosseum users. The default username and password are `root` and `scope`.

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
For instance, the control message `1,0,0\n5,10,3` implements the scheduling policiy `1` for the first slice, and `0` for the second and third slices. The control message also sets `5` RBGs for the first slice, `10` for the second, and `3` for the third. (See the [radio_code](https://github.com/wineslab/colosseum-scope#radio_code) section of the SCOPE repository for more details on the meaning of these values.)

## Build file

Some configurations can be tuned directly from the [build_odu.sh](build_odu.sh) file:
- `RIC_HOST`: IP address of the near-real-time RIC to connect to. This is the IP of the LXC container running the near-real-time RIC in Colosseum, or of the e2term container in a local deployment
- `RIC_PORT`: port the near-real-time RIC is listening to
- `INTERFACE_TO_RIC`: interface used by the host to reach the near-real-time RIC
- `DEBUG`: if enabled (set to `1`), the E2 termination will transmit test metrics to the subscribed xApp. If disabled (set to `0`), run-time metrics corresponding to the served users will be transmitted. By default, these metrics are taken from the SCOPE CSV-formatted metrics [directory](https://github.com/wineslab/colosseum-scope/tree/main/radio_code/scope_config/metrics/csv). The freshness of these metrics can be tuned from the `DELTA_TS_S` in the [csv_reader.h](src/du_app/csv_reader.h) file.

## E2 metrics

The transmitted metrics can be selected by updating the `readMetricsInteractive` method in `csv_reader.c`. By default, we provide a profile that reports on a custom E2SM the following KPMs:
- `slice_id`: the ID of the slice on which the user is allocated
- `dl_buffer [bytes]`: the occupancy (in bytes) of the RLC buffer of bearer associated to the user
- `tx_brate downlink [Mbps]`: the transmit bitrate of the user, in downlink, in Mbps
- `ratio_granted_req_prb`: the ratio of granted and requested PRBs for the user
- `slice_prb`: the number of PRBs of the slice
- `tx_pkts_downlink`: the number of packets transmitted in downlink by the user

A complete list of KPMs can be found in the `struct bs_metrics` of the [`csv_reader.h`](https://github.com/wineslab/colosseum-scope-e2/blob/main/src/du_app/csv_reader.h) file, and different reports can be generated by adding more cases to the the `switch` on the `metrics_preset` variable.

## Maximum payload of data reports and message segmentation

Large messages over the E2 interface may cause crashes in the `e2term` component upon decoding of the ASN.1 structures. For this reason, the E2 reports are split if their size is larger than the `MAX_REPORT_PAYLOAD` macro (currently set to 300 bytes). In this case, the partial reports (except the last one) will start with the character `m`.

## Troubleshooting

- Cannot set processes with real-time priority: If running in an LXC container on a testbed external to Colosseum, make sure that your LXC container is allowed to run real-time processes (option `limits.kernel.rtprio: "99"` in the container configuration). On Colosseum, this option is applied by default.

---
<sup><a id="footnote1">1</a></sup>Omit the second line of the control message to only transmit a scheduling policy control, e.g., `1,0,0`. Start the control message with a newline character (`\n`) to only transmit a slicing allocation control, e.g., `\n5,10,3`
