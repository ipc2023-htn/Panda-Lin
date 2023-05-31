# Participant: $\text{Lifted-Lin}_{\text{\\{Lilotane, Lifted-PANDA\\}}}^{\text{Simple-Precs/Effs}}$
This is the participant $\text{Lifted-Lin}_{\text{\\{Lilotane, Lifted-PANDA\\}}}^{\text{Simple-Precs/Effs}}$. 

Ying Xian Wu \<yingxian.wu@anu.edu.au\>, Songtuan Lin \<songtuan.lin@anu.edu.au\>, Pascal Bercher \<pascal.bercher@anu.edu.au\>

### Compiling the Code
The instructions for compiling the code is identical to those for PANDA, i.e.,
```
python build.py
```
### Running the Code
```
python planner.py --domain path_to_domain.hddl --task path_to_task.hddl
```

## Overview of All Known Participants

https://docs.google.com/spreadsheets/d/1SekZKjNuVmWJgy_UxavCPydoqVcYevxCrwu3eQBxmSU/edit#gid=0


## Participation Information

### PO-agile Track, Participants:

- $\text{Agi-Grounded-Lin}_{\text{PANDA}}^{\text{\\{Complex, Simple\\}-Precs/Effs}}$
  * Configuration 1 -- $\text{Agi-Grounded-Lin}_{\text{PANDA}}^{\text{Simple-Precs/Effs}}$:
    + Linearizer: simple inference
    + Search engine: greedy-A* + RC(Add)
  * Configuration 2 -- $\text{Agi-Grounded-Lin}_{\text{PANDA}}^{\text{Complex-Precs/Effs}}$:
    + Linearizer: complex inference
    + Search engine: greedy-A* + RC(Add)
  * Configuration 3 -- $\text{Agi-Grounded-Lin}_{\text{PANDA}}^{\text{Complex-Precs/Effs}}$
    + Linearizer: complex inference
    + Search engine: greedy-A* + RC(FF)
- $\text{Lifted-Lin}_{\text{\\{Lilotane, Lifted-PANDA\\}}}^{\text{Simple-Precs/Effs}}$
  * Configuration 1 -- $\text{Lifted-Lin}_{\text{Lilotane}}^{\text{Simple-Precs/Effs}}$
  * Configuration 2 -- $\text{Lifted-Lin}_{\text{Lifted-PANDA}}^{\text{Simple-Precs/Effs}}$


### PO-satisficing Track, Participants:

- $\text{Sat-Grounded-Lin}_{\text{PANDA}}^{\text{\\{Complex, Simple\\}-Precs/Effs}}$
  * Configuration 1 -- $\text{Sat-Grounded-Lin}_{\text{PANDA}}^{\text{Simple-Precs/Effs}}$:
    + Linearizer: simple inference
    + Search engine: greedy-A* + RC(Add), then greedy-A* + RC(FF), then A* + RC(LM-cut)) 
  * Configuration 2 -- $\text{Sat-Grounded-Lin}_{\text{PANDA}}^{\text{Simple-Precs/Effs}}$:
    + Linearizer: simple inference
    + Search engine: A* + RC(LM-cut)
  * Configuration 3 -- $\text{Sat-Grounded-Lin}_{\text{PANDA}}^{\text{Complex-Precs/Effs}}$ 
    + Linearizer: complex inference
    + Search engine: A* + RC(LM-cut)
- $\text{Lifted-Lin}_{\text{\\{Lilotane, Lifted-PANDA\\}}}^{\text{Simple-Precs/Effs}}$
  * Configuration 1: $\text{Lifted-Lin}_{\text{Lilotane}}^{\text{Simple-Precs/Effs}}$
  * Configuration 2: $\text{Lifted-Lin}_{\text{Lifted-PANDA}}^{\text{Simple-Precs/Effs}}$

## Participants

### Participant 1 -- $\text{Agi-Grounded-Lin}_{\text{PANDA}}^{\text{\\{Complex, Simple\\}-Precs/Effs}}$:

This participant consists of an inner planner and an outer one, both of which are built upon the PANDA search engine. There are three configurations for this participant each of which corresponds to a setting for the inner planner.

**Inner Planner**:
- Configuration 1:
    + Pruning technique for TOHTN planning by Conny.
    + Linearizing technique by Ying (linearizer)
        * Naive inference approach for inferring compound tasks' preconditions and effects.
    + PANDA search engine:
        * Heuristic: RC(Add)
        * Search strategy: greedy best A* with w=2
        * Visited list: default 
- Configuration 2:
    + Pruning technique for TOHTN planning by Conny.
    + Linearizing technique by Ying (linearizer)
        * Complex inference approach for inferring compound tasks' preconditions and effects.
    + PANDA search engine:
        * Heuristic: RC(Add)
        * Search strategy: greedy best A* with w=2
        * Visited list: default 
- Configuration 3:
    + Pruning technique for TOHTN planning by Conny.
    + Linearizing technique by Ying (linearizer)
        * Naive inference approach for inferring compound tasks' preconditions and effects.
    + PANDA search engine:
        * Heuristic: RC(FF)
        * Search strategy: greedy best A* with w=2
        * Visited list: default 

**Outer Planner**:
- PANDA search engine:
    * Heuristic: RC(FF)
    * Search strategy: weighted A* with weight=2 and gValue=gValMixed (**TODO**: Is this correct?) <== I asked Gregor.
    * Visited list: default 
    * The outer planner will only be called if the inner planner fails to find a solution

### Participant 2 -- $\text{Sat-Grounded-Lin}_{\text{PANDA}}^{\text{\\{Complex, Simple\\}-Precs/Effs}}$:
This participant again consists of an inner planner and an outer one. There are also three configurations for this participant each of which corresponds to a setting for the inner planner.

**Inner Planner**:
- Configuration 1:
    + Pruning technique for TOHTN planning by Conny.
    + Linearizing technique by Ying (linearizer)
        * Naive inference approach for inferring compound tasks' preconditions and effects.
    + PANDA search engine -- a three-rounds search:
        * First-round search:
            + Heuristic: RC(Add)
            + Search strategy: greedy best A* with w=2
            + Visited list: default
        * Second-round search:
            + Heuristic: RC(FF)
            + Search strategy: greedy best A* with w=2
            + Visited list: default 
            + The second-round search only happens if the first-round returns a solution.
            + This second-round search kills all search nodes whose f-value is greater than the cost of the solution returned by the first-round search.
        * Third-round search:
            + Heuristic: RC(LM-cut)
            + Search strategy: A* (cost-optimal)
            + Visited list: default 
            + The third-round search only happens if the first and second rounds return find a solution.
- Configuration 2:
    + Pruning technique for TOHTN planning by Conny.
    + Linearizing technique by Ying (linearizer)
        * Naive inference approach for inferring compound tasks' preconditions and effects.
    + PANDA search engine:
        + Heuristic: RC(LM-cut)
        + Search strategy: A* (cost-optimal)
        + Visited list: default
- Configuration 3:
    + Pruning technique for TOHTN planning by Conny.
    + Linearizing technique by Ying (linearizer)
        * Complex inference approach for inferring compound tasks' preconditions and effects.
    + PANDA search engine:
        + Heuristic: RC(LM-cut)
        + Search strategy: A* (cost-optimal)
        + Visited list: default

**Outer Planner**:
+ PANDA search engine:
    * Heuristic: RC(FF)
    * Search strategy: weighted A* with weight=2 and gValue=gValMixed (**TODO**: Is this correct?) <== I asked Gregor.
    * Visited list: default 
    * The outer planner will only be called if the inner planner fails to find a solution

    
    
### Participant 3 -- $\text{Lifted-Lin}_{\text{\\{Lilotane, Lifted-PANDA\\}}}^{\text{Simple-Precs/Effs}}$:
This participant again consists of an inner planner and an outer one. There are two configurations for it each of which selects a different search engine for the inner planner.

**Inner planner**:
+ Configuration 1:
    * Lifted linearizing technique by Ying (linearizer).
    * Search engine: Lilotane with Glucose as the SAT solver (claimed to work best in the IPC 2020)
+ Configuration 2:
    * Lifted linearizing technique by Ying (linearizer).
    * Search engine: Lifted-PANDA

**Outer planner**:
+ Search engine: Lifted-PANDA
