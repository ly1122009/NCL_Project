## Summary


## Type of change
- [ ] Feature
- [ ] Bug fix
- [ ] Refactor
- [ ] Test
- [ ] CI/Build
- [ ] Docs

## Self-review checklist
- [ ] Builds & `ctest` passes locally under `gcc-dev-Debug`
- [ ] Builds & `ctest` passes locally under `gcc-dev-ASan` (required if touching threading/memory/OSAL)
- [ ] Ran `clang-format` / `clang-tidy` and reviewed the diagnostics (currently non-blocking in CI, see docs/WORKFLOW.md)
- [ ] Added/updated unit tests for new behavior
- [ ] Does this need a real Raspberry Pi + webcam check before merging to `main`? If yes, why, and has `hw-in-loop.yml` been run (`workflow_dispatch`)?

## Notes for reviewer (future me)

