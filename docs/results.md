## Genetic Algorithm

### Relative inputs, hidden layers = [16]

#### Configuration

Hyperparameters:

**population size:** 200 <br>
**tournament size:** 5 <br>
**elite count:** 5 <br>
**hidden layers:** [16] <br>
**mutation rate:** 0.10 <br>
**mutation strength:** 0.20 <br>
**min mutation strength:** 0.05 <br>
**mutation strength dropoff:** 40

Input Parameters:

**relative apple** <br>
**direction** <br>
**snake size** <br>
**distance to walls** <br>
**distance to danger** <br>

#### results

| Generation | Max Score | Average Score | Average Steps | Average Fitness | Elapsed Time | Elapsed Time / Generaion |
| ---------- | --------- | ------------- | ------------- | --------------- | ------------ | ------------------------ |
| 1000       | 122       | 76.68         | 2875.76       | 73909.52        | 25.2s        | 0.0252s                  |
| 2000       | 128       | 89.62         | 3344.99       | 86372.81        | 38.6s        | 0.0386s                  |
| 3000       | 134       | 98.95         | 3965.92       | 95084.87        | 43.4s        | 0.0434s                  |
| 4000       | 140       | 103.54        | 4156.23       | 99483.01        | 45.5s        | 0.0455s                  |
| 5000       | 146       | 105.85        | 4304.56       | 101639.35       | 46.9s        | 0.0469s                  |
| 7500       | 179       | 125.45        | 5965.08       | 119580.10       | 145.3s       | 0.0581s                  |
| 10000      | 262       | 175.85        | 10674.15      | 165268.44       | 259.3s       | 0.1037s                  |
| 15000      | 286       | 215.29        | 13756.69      | 201662.00       | 804.1s       | 0.1608s                  |
| 20000      | 295       | 224.71        | 14638.88      | 210164.83       | 890.0s       | 0.1780s                  |
| 25000      | 304       | 236.83        | 16389.24      | 220535.74       | 985.9s       | 0.1972s                  |


### Full Grid, hidden layers = [20]

Hyperparameters:

**population size:** 700 <br>
**tournament size:** 9 <br>
**elite count:** 18 <br>
**hidden layers:** [20] <br>
**mutation rate:** 0.02 <br>
**mutation strength:** 0.25 <br>
**min mutation strength:** 0.03 <br>
**mutation strength dropoff:** 1000

Input Parameters:

**full board**

| Generation | Max Score | Average Score | Average Steps | Average Fitness | Elapsed Time | Elapsed Time / Generaion |
| ---------- | --------- | ------------- | ------------- | --------------- | ------------ | ------------------------ |
| 1000       | 4         | 0.10          | 333.66        | 361.97          | 169.7s       | 0.1697s                  |
| 2000       | 4         | 0.12          | 316.34        | 361.73          | 168.1s       | 0.1681s                  |
| 3000       | 4         | 0.17          | 283.65        | 358.60          | 167.9s       | 0.1679s                  |
| 4000       | 4         | 0.19          | 265.27        | 356.88          | 166.6s       | 0.1666s                  |
| 5000       | 4         | 0.20          | 239.91        | 342.06          | 166.2s       | 0.1662s                  |
| 7500       | 4         | 0.21          | 258.15        | 354.13          | 415.2s       | 0.1661s                  |
| 10000      | 4         | 0.24          | 228.67        | 344.58          | 415.4s       | 0.1662s                  |

### Full Grid, hidden layers = [20]
