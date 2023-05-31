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
