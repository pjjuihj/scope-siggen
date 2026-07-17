# Triage Labels

## Label Mapping

| Role | Label | Description |
|------|-------|-------------|
| needs-triage | `needs-triage` | Maintainer needs to evaluate |
| needs-info | `needs-info` | Waiting on reporter for more information |
| ready-for-agent | `ready-for-agent` | Fully specified, AFK-ready (agent can pick up with no human context) |
| ready-for-human | `ready-for-human` | Needs human implementation |
| wontfix | `wontfix` | Will not be actioned |

## State Machine

```
New issue → needs-triage
    ├── needs-info (incomplete report)
    ├── ready-for-agent (fully specified)
    ├── ready-for-human (needs human decision)
    └── wontfix (out of scope)
```
