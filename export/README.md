# Binary Ninja Watcher Exports

This directory contains local text dumps for people watching or reviewing the
reconstruction from outside Binary Ninja. They are not authoritative
reconstruction evidence and are not required by the normal build, test, doctor,
or plan-marker workflow.

Current files are generated snapshots from the local Binary Ninja database, such
as `text.hlil.txt`, `data.linear.txt`, `rdata.linear.txt`, `rsrc.linear.txt`,
and `user_types.txt`. Treat them as convenience views that can drift until
regenerated.

`tools/recoil_zinterp_dispatch_audit.py` can compare the zInterp dispatcher
source against these dumps when they are present, but Binary Ninja remains the
source of truth for reconstruction work.
