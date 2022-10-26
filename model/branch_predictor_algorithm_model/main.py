#!/usr/bin/python3
from http.server import executable
from tqdm import tqdm
from static_always_jump import static_always_jump
from static_always_not_jump import static_always_not_jump
from gshare import gshare
from direct_mapped_simple_btb import direct_mapped_simple_btb

branch_predictor_list = [static_always_jump(), static_always_not_jump(), gshare(6), gshare(8), gshare(10), gshare(15), gshare(20)]
branch_target_buffer_list = [direct_mapped_simple_btb(256)]

dump_path = "../../branch_dump/coremark_10.txt"

print("Reading dump file...")

with open(dump_path, "r") as f:
    dump_txt = f.read()

print("Parsing dump file...")

lines = dump_txt.split('\n')
condition_branch_info = []
branch_info = []

for i in tqdm(range(0, len(lines) - 1), colour="blue"):
    line = lines[i]
    arg = line.split(',')

    if len(arg) != 2:
        raise Exception("branch dump file error")

    pc = int(arg[0], 16)
    inst = int(arg[1], 16)

    opcode = inst & 0x7f
    rd = (inst >> 7) & 0x1f
    funct3 = (inst >> 12) & 0x7
    rs1 = (inst >> 15) & 0x1f
    rs2 = (inst >> 20) & 0x1f
    funct7 = (inst >> 25) & 0x7f
    rd_is_link = (rd == 1) or (rd == 5)
    rs1_is_link = (rs1 == 1) or (rs1 == 5)

    next_line = lines[i + 1]
    next_arg = next_line.split(',')

    if next_line.strip() == "":
        continue
    
    if len(next_arg) != 2:
        raise Exception("branch dump file error")

    next_pc = int(next_arg[0], 16)

    if opcode == 0x63:
        if next_pc == pc + 4:
            #[pc, not jump]
            condition_branch_info.append([pc, False])
        else:
            #[pc, jump]
            condition_branch_info.append([pc, True])
    
    if opcode == 0x6f or opcode == 0x67 or opcode == 0x63:
        is_branch = True

        if opcode == 0x67 and not rd_is_link and rs1_is_link:
            is_ret = True
        else:
            is_ret = False
    else:
        is_branch = False
        is_ret = False
    
    branch_info.append([pc, next_pc, is_branch, is_ret])

for branch_predictor in branch_predictor_list:
    print(branch_predictor.get_name() + " is processing...")
    
    hit = 0
    miss = 0

    for i in tqdm(range(0, len(condition_branch_info)), colour="blue", leave=False):
        pc = condition_branch_info[i][0]
        jump = condition_branch_info[i][1]

        predict = branch_predictor.get(pc)

        if predict == jump:
            hit += 1
        else:
            miss += 1

        branch_predictor.update(pc, jump, predict == jump)

    print("[" + branch_predictor.get_name() + "] hit: " + str(hit) + ", miss: " + str(miss) + ", total: " + str(hit + miss) + ", hit rate: " + str(hit * 100 / (hit + miss)) + "%")

for branch_target_buffer in branch_target_buffer_list:
    print(branch_target_buffer.get_name() + " is processing...")
    
    hit = 0
    miss = 0
    ret = 0

    for i in tqdm(range(0, len(branch_info)), colour="blue", leave=False):
        pc = branch_info[i][0]
        next_pc = branch_info[i][1]
        is_branch = branch_info[i][2]
        is_ret = branch_info[i][3]

        if is_ret:
            ret += 1

        predict = branch_target_buffer.get(pc)

        if predict == next_pc:
            hit += 1
        else:
            miss += 1

        branch_target_buffer.update(pc, next_pc, is_branch, is_ret)

    print("[" + branch_target_buffer.get_name() + "] hit: " + str(hit) + ", miss: " + str(miss) + ", ret:" + str(ret) + ", total: " + str(hit + miss) + ", hit rate: " + str(hit * 100 / (hit + miss)) + "%")