# Overview of All Known Participants

[Online Table (clickable link)](https://docs.google.com/spreadsheets/d/1SekZKjNuVmWJgy_UxavCPydoqVcYevxCrwu3eQBxmSU/edit#gid=0)

# Summary of Participating Planners

$\text{Grounded-Lin}^{\text{Complex-Precs/Effs}}_{\text{Lookahead}}$

+ Configuration 1:
    * Linearizer with the complex inference approach
    * Engine: pandaPI with lookahead
        + Heuristic: rc2(add)
        + Search algorithm: GBFS
+ Configuration 2:
    * Linearizer with the complex inference approach
    * Engine: pandaPI with lookahead
        + Heuristic: rc2(ff)
        + Search algorithm: GBFS
+ Configuration 3:
    * Linearizer with the complex inference approach
    * Engine: pandaPI with lookahead
        + First-round search:
            * Heuristic: rc2(add)
            * Search algorithm: GBFS
        + Second-round search:
            * Heuristic: rc2(ff)
            * Search algorithm: weighted A* with $w=2$
        + Thrid-round search:
            * Heuristic: rc2(lmcut)
            * Search algorithm: A* (optimal search)

$\text{Grounded-Lin}^{\text{Simple-Precs/Effs}}_{\text{PANDA}}$

+ Configuration 1:
    * Linearizer with the simple inference approach
    * Engine: pandaPI without lookahead
        + Heuristic: rc2(add)
        + Search algorithm: GBFS
+ Configuration 2:
    * Linearizer with the simple inference approach
    * Engine: pandaPI without lookahead
        + Heuristic: rc2(ff)
        + Search algorithm: GBFS
+ Configuration 3:
    * Linearizer with the simple inference approach
    * Engine: pandaPI without lookahead
        + First-round search:
            * Heuristic: rc2(add)
            * Search algorithm: GBFS
        + Second-round search:
            * Heuristic: rc2(ff)
            * Search algorithm: weighted A* with $w=2$
        + Thrid-round search:
            * Heuristic: rc2(lmcut)
            * Search algorithm: A* (optimal search)

$\text{Lifted-Lin}^{\text{Simple-Precs/Effs}}$
  * Configuration 1:
    + Lifted linearizer
    + Search engine: Lilotane
  * Configuration 2:
    + Lifted linearizer
    + Search engine: pandaPI with SAT
  * Configuration 3:
    + Lifted linearizer
    + Search engine: pandaPI with SAT (optimal version) 

# Participanting Planners (Details):

## Participant 1 -- $\text{Agi-Grounded-Lin}_{\text{PANDA}}^{\text{\\{Complex, Simple\\}-Precs/Effs}}$:

This participant consists of an inner planner and an outer one, both of which are built upon the PANDA search engine. There are three configurations for this participant each of which corresponds to a setting for the inner planner.

**Inner Planner**:

- Configuration 1:
    + Pruning technique for TOHTN planning by Conny.
    + Linearizing technique by Ying (linearizer).
        * Complex inference approach for inferring compound tasks' preconditions and effects.
    + Search engine: pandaPI
        * Heuristic: RC(Add)
        * Search strategy: greedy best first search
        * Visited list: default 
- Configuration 2:
    + Pruning technique for TOHTN planning by Conny.
    + Linearizing technique by Ying (linearizer).
        * Complex inference approach for inferring compound tasks' preconditions and effects.
    + Search engine: pandaPI
        * Heuristic: RC(Add)
        * Search strategy: greedy best first search
        * Visited list: default 
- Configuration 3:
    + Linearizing technique by Ying (linearizer).
        * Naive inference approach for inferring compound tasks' preconditions and effects.
    + search engine: pandaPI
        * Heuristic: RC(Add)
        * Search strategy: greedy best first search
        * Visited list: default 

**Outer Planner**:
- The outer planner will only be called if the inner planner fails to find a solution.
- Search engine: pandaPI
    * Heuristic: RC(FF)
    * Search strategy: greedy A* (w = 2)
    * Visited list: default 

## Participant 2 -- $\text{Sat-Grounded-Lin}_{\text{PANDA}}^{\text{\\{Complex, Simple\\}-Precs/Effs}}$:
This participant again consists of an inner planner and an outer one. There are also three configurations for this participant each of which corresponds to a setting for the inner planner.

**Inner Planner**:

- Configuration 1:
    + Pruning technique for TOHTN planning by Conny.
    + Linearizing technique by Ying (linearizer).
        * Complex inference approach for inferring compound tasks' preconditions and effects.
    + Search engine: pandaPI (a three-rounds search)
        * First-round search:
            + Heuristic: RC(Add)
            + Search strategy: greedy best first search
            + Visited list: default
        * Second-round search:
            + Heuristic: RC(FF)
            + Search strategy: greedy A* (w = 2)
            + Visited list: default 
            + The second-round search only happens if the first-round returns a solution.
            + This second-round search kills all search nodes whose f-value is greater than the cost of the solution returned by the first-round search.
        * Third-round search:
            + Heuristic: RC(FF)
            + Search strategy: greedy A* (w = 1.5)
            + Visited list: default 
            + The third-round search only happens if the first and second rounds return a solution.
- Configuration 2:
    + Pruning technique for TOHTN planning by Conny.
    + Linearizing technique by Ying (linearizer).
        * Complex inference approach for inferring compound tasks' preconditions and effects.
    + Search engine: pandaPI
        * First-round search:
            + Heuristic: RC(FF)
            + Search strategy: greedy A* (w = 2)
            + Visited list: default
        * Second-round search:
            + Heuristic: RC(FF)
            + Search strategy: greedy A* (w = 1.5)
            + Visited list: default 
            + The second-round search only happens if the first-round returns a solution.
            + This second-round search kills all search nodes whose f-value is greater than the cost of the solution returned by the first-round search.
        * Third-round search:
            + Heuristic: RC(LM-cut)
            + Search strategy: A* (cost-optimal)
            + Visited list: default 
            + The third-round search only happens if the first and second rounds return a solution.
- Configuration 3:
    + Linearizing technique by Ying (linearizer).
        * Navie inference approach for inferring compound tasks' preconditions and effects.
    + Search engine: pandaPI
        + Heuristic: RC(LM-cut)
        + Search strategy: A* (cost-optimal)
        + Visited list: default

**Outer Planner**:
+ The outer planner will only be called if the inner planner fails to find a solution.
+ Search engine: pandaPI
    * Heuristic: RC(FF)
    * Search strategy: greedy A* (w=2)
    * Visited list: default 

    
    
## Participant 3 -- $\text{Lifted-Lin}_{\text{Lilotane}}^{\text{Simple-Precs/Effs}}$:
This participant again consists of an inner planner and an outer one. There are two configurations for it each of which selects a different search engine for the inner planner.

**Inner planner**:

+ Configuration 1:
    * Lifted linearizing technique by Ying (linearizer).
    * Search engine: Lilotane with Glucose as the SAT solver (claimed to work best in the IPC 2020)

**Outer planner**:

+ Search engine: pandaPI
    * Heuristic: RC(FF)
    * Search strategy: greedy A*
