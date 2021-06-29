# -*- coding: utf-8 -*-
"""
Created on Fri Jun 25 11:36:44 2021

@author: jared
"""
import csv

with open('test_data_pre1.csv', newline='') as csvread:
    with open("test_data_post1.csv","a") as csvwrite:
        writer = csv.writer(csvwrite,delimiter=",")
        reader = csv.reader(csvread)
        for row in reader:
#            data = row[0]
            decoded_bytes = ""
            for e in row:
                decoded_bytes += e
            data = (decoded_bytes[2:len(decoded_bytes)-5]).split(", ")
            print(data[0])
            if len(data) == 3:
                writer.writerow([data[0],data[1],data[2]])