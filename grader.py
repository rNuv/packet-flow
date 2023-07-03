import os
import sys
import time
import json
import argparse
import subprocess
import signal

parser = argparse.ArgumentParser()
parser.add_argument('-t', metavar='n', type=int, nargs=1, default=None, help='test Case #n')
parser.add_argument('inpath', metavar='inpath', type=str, nargs='?', default='./', help='path to the code directory')
parser.add_argument('outpath', metavar='outpath', type=str, nargs='?',
                    default='./', help='path to the result directory')
args = parser.parse_args()

os.chdir(args.inpath)

if not os.path.exists("./configs/"):
    print("Cannot find ./configs/")
    exit(0)

if not os.path.exists("./data/"):
    os.mkdir("./data/")


configTemp1 = ""
configTemp2 = ""
FINWAIT_TIME = 20


def killprocs():
    os.system("pkill --signal 9 -f Sender >/dev/null 2>&1")
    os.system("pkill --signal 9 -f Receiver >/dev/null 2>&1")
    os.system("pkill --signal 9 -f relayer >/dev/null 2>&1")
    time.sleep(1)


def killed(signum, frame):
    killprocs()
    exit(0)


signal.signal(signal.SIGTERM, killed)
signal.signal(signal.SIGINT, killed)


def genConfig(i, param):
    if param["pairs"] == 1:
        config = configTemp1
    else:
        config = configTemp2
    filename = "./configs/config%d.xml" % (i+1)

    config = config.replace("%{bandwidth}%", str(param["bandwidth"]))
    config = config.replace("%{propagation_delay}%", str(param["delay"]))
    config = config.replace("%{buffer_size}%", str(param["buffer"]))

    with open(filename, "w") as fout:
        fout.write(config)

    return filename


def genData(band, t):
    size = int(band*t/1000/8)
    filename = "./data/%dM.txt" % size
    size = size*1024*1024
    if not os.path.exists(filename) or (os.path.getsize(filename) != size):
        cmd = "./generator %d %s" % (size, filename)
        generator = subprocess.Popen(cmd.split(), stdout=subprocess.DEVNULL, stderr=sys.stderr)
        generator.wait()
    return size, filename


def getOutput(out):
    lines = out.decode().split('\n')
    output = []
    for line in lines:
        if line.find("kbps") != -1:
            output.append(line)
    return output


def gradeBand(band, std):
    if band >= std*0.85:
        return 10
    else:
        return round(10.0*band/(std*0.85))


def runBandwidth(i, config, param):
    datasize, datafile = genData(param["bandwidth"]/2, param["timeout"]-20)

    try:
        cmd = "./relayer %s" % config
        relayer = subprocess.Popen(cmd.split(), stdout=subprocess.DEVNULL, stderr=sys.stderr)
        cmd = "./Receiver temp.txt -p %d" % 20000
        receiver = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE, stderr=sys.stderr)
        cmd = "./Sender  %s -p %d -r %d" % (datafile, 10000, 50001)
        sender = subprocess.Popen(cmd.split(), stdout=subprocess.DEVNULL, stderr=sys.stderr)
    except Exception as e:
        killprocs()
        return 0

    try:
        out1, err = receiver.communicate(timeout=param["timeout"])
        sender.communicate(timeout=param["timeout"])
    except Exception as e:
        receiver.kill()
        sender.kill()
        relayer.kill()
        if Exception is subprocess.TimeoutExpired:
            print("Timed out after %d seconds" % param["timeout"])
        else:
            print(e)
        return 0

    relayer.kill()
    relayer.communicate()

    cmd = "diff -q %s temp.txt" % datafile
    with subprocess.Popen(cmd.split(), stdout=subprocess.PIPE, stderr=sys.stderr) as proc:
        out, err = proc.communicate()
        if len(out) != 0:
            print("Corrupted file received")
            return 0

    try:
        line = getOutput(out1)[-1]
        band = float((line.split())[1])
    except Exception as e:
        return 0
    return band


def fairnessIndex(x1, x2):
    if x1 == 0 and x2 == 0:
        return 0.5
    return 1.0*(x1+x2)*(x1+x2)/(2*(x1*x1+x2*x2))


def gradeFairness(f):
    if f >= 0.95:
        return 10
    return round(10*(f-0.5)/(0.95-0.5))


covg = 5
wind = 30
fairTestTime = covg+covg+wind+covg+covg


def getFairness(out1, out2):
    seq1 = []
    for line in out1[:-1]:
        [t, x, s] = line.split()
        seq1.append([int(t), float(x)])
    l1 = len(seq1)-covg

    seq2 = []
    for line in out2[:-1]:
        [t, x, s] = line.split()
        seq2.append([int(t), float(x)])
    l2 = len(seq2)-covg

    i = 0
    j = covg-1
    while i < l1 and seq1[i][0] < seq2[j][0]:
        i += 1

    s1 = 0
    s2 = 0
    k = 0
    while i+k < l1 and j+k < l2:
        # print(seq1[i+k][1], seq2[j+k][1], fairnessIndex(seq1[i+k][1], seq2[j+k][1]))
        s1 += seq1[i+k][1]
        s2 += seq2[j+k][1]
        k += 1
    return fairnessIndex(s1, s2)

    # s1 = 0
    # s2 = 0
    # sumf = 0
    # for k in range(0, wind):
    #     sumf += fairnessIndex(seq1[i+k][1], seq2[j+k][1])
    #     # s1 += seq1[i+k][1]
    #     # s2 += seq2[j+k][1]

    # res = max(0.5, sumf/wind)
    # # res = max(0.5, fairnessIndex(s1, s2))
    # while i+wind < l1 and j+wind < l2:
    #     sumf -= fairnessIndex(seq1[i][1], seq2[j][1])
    #     sumf += fairnessIndex(seq1[i+wind][1], seq2[j+wind][1])
    #     res = max(res, sumf/wind)
    #     # s1 -= seq1[i][1]
    #     # s1 += seq1[i+wind][1]
    #     # s2 -= seq2[j][1]
    #     # s2 += seq2[j+wind][1]
    #     # res = max(res, fairnessIndex(s1, s2))
    #     i += 1
    #     j += 1

    # return res


def runFairness(i, config, param):
    datasize, datafile = genData(param["bandwidth"]/2, fairTestTime)

    try:
        cmd = "./relayer %s" % config
        relayer = subprocess.Popen(cmd.split(), stdout=subprocess.DEVNULL, stderr=sys.stderr)

        cmd = "./Receiver temp1.txt -p %d" % 20000
        receiver1 = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE, stderr=sys.stderr)
        cmd = "./Sender  %s -p %d -r %d" % (datafile, 10000, 50001)
        sender1 = subprocess.Popen(cmd.split(), stdout=subprocess.DEVNULL, stderr=sys.stderr)
        time.sleep(covg)

        cmd = "./Receiver temp2.txt -p %d" % 40000
        receiver2 = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE, stderr=sys.stderr)
        cmd = "./Sender  %s -p %d -r %d" % (datafile, 30000, 50003)
        sender2 = subprocess.Popen(cmd.split(), stdout=subprocess.DEVNULL, stderr=sys.stderr)
    except Exception as e:
        killprocs()
        return 0.5

    try:
        out2, err = receiver2.communicate(timeout=param["timeout"])
        out1, err = receiver1.communicate(timeout=param["timeout"])
        sender2.communicate(timeout=param["timeout"])
        sender1.communicate(timeout=param["timeout"])
    except Exception as e:
        receiver2.kill()
        receiver1.kill()
        sender2.kill()
        sender1.kill()
        relayer.kill()
        if Exception is subprocess.TimeoutExpired:
            print("Timed out after %d seconds" % param["timeout"])
        else:
            print(e)
        return 0.5

    relayer.kill()
    relayer.communicate()

    cmd = "diff -q %s temp1.txt" % datafile
    with subprocess.Popen(cmd.split(), stdout=subprocess.PIPE, stderr=sys.stderr) as proc:
        out, err = proc.communicate()
        if len(out) != 0:
            print("Corrupted file received")
            return 0.5

    cmd = "diff -q %s temp2.txt" % datafile
    with subprocess.Popen(cmd.split(), stdout=subprocess.PIPE, stderr=sys.stderr) as proc:
        out, err = proc.communicate()
        if len(out) != 0:
            print("Corrupted file received")
            return 0.5

    return getFairness(getOutput(out1), getOutput(out2))


killprocs()

with open("./configs/template1.xml", "r") as fin:
    configTemp1 = fin.read()
with open("./configs/template2.xml", "r") as fin:
    configTemp2 = fin.read()

params = [
    {"pairs": 1, "bandwidth": 2500, "delay": 40, "buffer": 25, "timeout": 60},
    {"pairs": 1, "bandwidth": 2000, "delay": 80, "buffer": 40, "timeout": 60},
    {"pairs": 1, "bandwidth": 800, "delay": 125, "buffer": 25, "timeout": 60},
    {"pairs": 1, "bandwidth": 10000, "delay": 22, "buffer": 55, "timeout": 60},
    {"pairs": 1, "bandwidth": 15000, "delay": 20, "buffer": 75, "timeout": 60},
    {"pairs": 2, "bandwidth": 2500, "delay": 40, "buffer": 25, "timeout": 2*fairTestTime+20},
    {"pairs": 2, "bandwidth": 2000, "delay": 80, "buffer": 40, "timeout": 2*fairTestTime+20},
    {"pairs": 2, "bandwidth": 800, "delay": 125, "buffer": 25, "timeout": 2*fairTestTime+20},
    {"pairs": 2, "bandwidth": 10000, "delay": 22, "buffer": 55, "timeout": 2*fairTestTime+20},
    {"pairs": 2, "bandwidth": 15000, "delay": 20, "buffer": 75, "timeout": 2*fairTestTime+20}
]


def runTestCase(i):
    param = params[i]
    case = {
        "score": 0,
        "max_score": 10,
        "visibility": "visible",
        "output": json.dumps(param)
    }

    print("Case %d:" % (i+1), param)
    config = genConfig(i, param)

    if param["pairs"] == 1:
        band = runBandwidth(i, config, param)
        points = gradeBand(band, param["bandwidth"])
        print("Bandwidth: %.2fkb/s" % band)
    else:
        fair = runFairness(i, config, param)
        points = gradeFairness(fair)
        print("Fairness: %.2lf" % fair)

    case["score"] += points
    print("Points: %d/10\n" % points)
    return case


if args.t is not None:
    runTestCase(args.t[0]-1)
    exit(0)

cases = []
score = 0
for i in range(0, 10):
    case = runTestCase(i)
    score += case["score"]
print("Total: %d/100" % score)

res = {
    "score": score,
    "stdout_visibility": "visible",
    "tests": cases
}

with open('%s/results.json' % args.outpath, 'w') as fout:
    fout.write(json.dumps(res))
