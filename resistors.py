import numpy as np

def resistorParallel(a, b):
    return (float(a) * float(b)) / (float(a) + float(b))

TESTSTART = 1
TESTRANGE = 30
STAGE2 = 1.
ranges = [15., 30., 50., 100.]
travels = [100., 250., 350., 500.]
r1Range = range(TESTSTART, TESTRANGE + TESTSTART)
r2Range = range(TESTSTART, TESTRANGE + TESTSTART)
r3Range = range(TESTSTART, TESTRANGE + TESTSTART)
r4Range = range(TESTSTART, TESTRANGE + TESTSTART)

values = np.zeros((len(ranges), len(travels), 4))

goodCombination = True
for r1 in r1Range:
    for r2 in r2Range:
        for r3 in r3Range:
            for r4 in r4Range:
                goodCombination = True
                for ran in range(len(ranges)):
                    for travel in range(len(travels)):
                        gains = [resistorParallel(r3, r4) / float(r1), float(r3) / float(r1), resistorParallel(r3, r4) / resistorParallel(r1, r2), float(r3) / resistorParallel(r1, r2)]
                        values[ran][travel][0] = 10.0 / 1.195 / STAGE2 / gains[0] * ranges[ran] / travels[travel]
                        values[ran][travel][1] = 10.0 / 1.195 / STAGE2 / gains[1] * ranges[ran] / travels[travel]
                        values[ran][travel][2] = 10.0 / 1.195 / STAGE2 / gains[2] * ranges[ran] / travels[travel] 
                        values[ran][travel][3] = 10.0 / 1.195 / STAGE2 / gains[3] * ranges[ran] / travels[travel]
                        # if r1 == 6 and r2 == 5 and r3 == 9 and r4 == 2:   # DEBUGGING
                        #     print(values[1][0])   # DEBUGGING
                        count = 0
                        for val in values[ran][travel]:
                            if 0.48 <= val <= 0.99:
                                count += 1
                        if count == 0:
                            goodCombination = False
                            break   # Break out of travel loop
                    if goodCombination is False:
                        break   # Break out of range loop
                if goodCombination is True:
                    print("Good combination: (" + str(r1) + ", " + str(r2) + ", " + str(r3) + ", " + str(r4) + ")")