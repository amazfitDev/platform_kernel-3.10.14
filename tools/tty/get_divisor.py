
import math
import string
import sys
import os
ext_clk = (24000000,26000000)

baud_rates = (
	50, 75, 110, 134, 150, 200, 300, 600,
	1200, 1800, 2400, 4800, 9600, 19200, 38400,
	57600, 115200, 230400, 460800, 500000, 576000,
	921600, 1000000, 1152000, 1500000, 2000000, 2500000,
	3000000, 3500000, 4000000
)

'''
    [baud_rate,div, umr, uacr]
'''
uart_baud_reg = []

def addelement(baud,div, umr, uacr):
    uart_baud_reg.append(baud)
    uart_baud_reg.append(div)
    uart_baud_reg.append(umr)
    uart_baud_reg.append(uacr)

def main(uartclk):
    global uart_baud_reg
    uart_baud_reg = []
    for n in range(len(baud_rates)):
        umr_best = div_best = uacr_best = 0
        uacr_count_best  = 0
        umr = 0
        div = 1
        tmp0 = []
        tmp1 = []
        for i in range(12):
            tmp0.append(0)
            tmp1.append(0)

        baud = baud_rates[n]

        if uartclk % (16 * baud) == 0:
            div_best = int(uartclk / (16 * baud))
            umr_best = 16
            uacr_best = 0
            addelement(baud,div_best,umr_best,uacr_best)
            continue

        while True:
            umr = int(uartclk / (baud * div))
            if umr > 32:
                div += 1
                continue
            if umr < 4:
                break
            for i in range(12):
                tmp0[i] = umr
                tmp1[i] = 0
                sum = 0
                for j in range(i + 1):
                    sum += tmp0[j]
                """
                t0 = 0x1000000000L
                t1 = (i * 1) * t0
                t2 = (sum * div) * t0
                t3 = div * t0
                t1 = t1 / baud
                t2 = t2 / uartclk
                t3 = t3 / (2 * uartclk)
                """
                t0 = 0x1000000000L
                t1 = (i + 1) *t0
                t2 = (sum * div) *t0
                t3 = div * t0
                t1 = int(t1 / baud)
                t2 = int(t2 / uartclk)
                t3 = int(t3 / (2 * uartclk))

                err = t1 - t2 - t3
                if err > 0:
                    tmp0[i] += 1
                    tmp1[i] = 1

            uacr = 0
            uacr_count = 0
            for i in range(12):
                if tmp1[i] == 1:
                    uacr |= 1 << i
                    uacr_count += 1


            if div_best == 0:
                div_best = div
                umr_best = umr
                uacr_best = uacr
                uacr_count_best = uacr_count
            '''
            the best value of umr should be near 16,
            and the value of uacr should better be smaller
            '''
            if abs(umr - 16) < abs(umr_best - 16) or \
                (abs(umr - 16) == abs(umr_best - 16) and uacr_best > uacr):
                div_best = div
                umr_best = umr
                uacr_best = uacr
                uacr_count_best = uacr_count

            div += 1

        if uacr_count_best != 0:
            uacr_best = 0
            for i in range(uacr_count_best,11,11/uacr_count_best):
                uacr_best |= 1 << i

        addelement(baud,div_best,umr_best,uacr_best)
def print_res():
    for i in range(0, len(uart_baud_reg), 4):
        print("\t{%d,0x%x,0x%x,0x%x}," %(uart_baud_reg[i],
                                         uart_baud_reg[i + 1],
                                         uart_baud_reg[i + 2],
                                         uart_baud_reg[i + 3])
              )

if __name__ == '__main__':
    dev_clk = 0

    if len(sys.argv) == 2:
        dev_clk = string.atoi(sys.argv[1:])
        main(dev_clk)
        print "=====================",
        print dev_clk
        print "====================="
        print_res()
    else:
        for i in range(len(ext_clk)):
            main(ext_clk[i])
            print "====================="
            print ext_clk[i]
            print "====================="
            print_res()

__author__ = 'bliu'
