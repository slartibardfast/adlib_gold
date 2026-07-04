# tests

The driver's test harness. The behavioural specs under `spec/` derive test obligations
(`allium plan`), dispositioned in each `spec/*.obligations` manifest. Spec-integrity
obligations are discharged `structural` by the allium check/analyse lane; the behavioural
obligations are `waived` here until a runnable harness exists.

A wave harness lands with the host's `plan/0004`: once the driver builds from the pinned
Windows 2000 DDK bundle (`call/0008`), the format-validation, resample, and dither
decisions are extracted into userspace-testable form, and the waived obligations convert
to `test:` dispositions that exercise those decision symbols.
