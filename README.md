# Data Packet
| DEC | HEX | Sample 1 | Sample 2 | Sample 3 | Sample 4 | Data                          |
| --- | --- | -------- | -------- | -------- | -------- | ----------------------------- |
| 0   | 0   | 02       | 02       | 02       | 02       |                               |
| 1   | 1   | 56       | 56       | 56       | 56       |                               |
| 2   | 2   | 00       | 00       | 00       | 00       |                               |
| 3   | 3   | 08       | 08       | 08       | 08       |                               |
| 4   | 4   | 40       | 40       | 40       | 40       |                               |
| 5   | 5   | 04       | 04       | 04       | 04       | bit4:Pump                        |
| 6   | 6   | 00       | 00       | 00       | 00       | Heater On                     |
| 7   | 7   | 00       | 00       | 00       | 00       |                               |
| 8   | 8   | 06       | 06       | 06       | 06       |                               |
| 9   | 9   | 04       | 04       | 04       | 04       |                               |
| 10  | A   | 00       | 00       | 00       | 00       |                               |
| 11  | B   | 02       | 02       | 02       | 0A       |                               |
| 12  | C   | 06       | 06       | 06       | 06       |                               |
| 13  | D   | 20       | 20       | 20       | 20       |                               |
| 14  | E   | 72       | 72       | 72       | 72       |                               |
| 15  | F   | 13       | 13       | 13       | 13       | bit6:TempInC                     |
| 16  | 10  | 00       | 00       | 00       | 00       |                               |
| 17  | 11  | 20       | 20       | 20       | 20       | 2 byte value                  |
| 18  | 12  | 1C       | 1C       | 1C       | 1C       | ^^^^
| 19  | 13  | 20       | 20       | 20       | 20       | 2 byte value<br>Copy Previous |
| 20  | 14  | 1C       | 1C       | 1C       | 1C       | ^^^^
| 21  | 15  | 20       | 20       | 20       | 20       | 2 byte value<br>Copy Previous |
| 22  | 16  | 1C       | 1C       | 1C       | 1C       | ^^^^
| 23  | 17  | 84       | 84       | 84       | 84       | 2 byte value                  |
| 24  | 18  | 03       | 03       | 03       | 03       | ^^^^
| 25  | 19  | 60       | 60       | 60       | 60       | 2 byte value                  |
| 26  | 1A  | 54       | 54       | 54       | 54       | ^^^^
| 27  | 1B  | 00       | 00       | 00       | 00       | Jets1                         |
| 28  | 1C  | 00       | 00       | 00       | 00       | Jets2                         |
| 29  | 1D  | 00       | 00       | 00       | 00       | Jets3                         |
| 30  | 1E  | 00       | 00       | 00       | 00       | Blower                        |
| 31  | 1F  | 31       | 31       | 31       | 31       | Heater Temp                   |
| 32  | 20  | 30       | 30       | 30       | 30       | ^^^^
| 33  | 21  | 33       | 33       | 33       | 30       | ^^^^
| 34  | 22  | 46       | 46       | 46       | 46       | ^^^^
| 35  | 23  | F7       | F7       | F7       | 86       | Heater Seconds                |
| 36  | 24  | 6B       | 6B       | 6B       | 69       | ^^^^
| 37  | 25  | 09       | 09       | 09       | 09       | ^^^^
| 38  | 26  | 00       | 00       | 00       | 00       | ^^^^
| 39  | 27  | 58       | 58       | 58       | 26       | Jets1 Seconds                 |
| 40  | 28  | 2B       | 2B       | 2B       | 26       | ^^^^
| 41  | 29  | 06       | 06       | 06       | 06       | ^^^^
| 42  | 2A  | 00       | 00       | 00       | 00       | ^^^^
| 43  | 2B  | E6       | EB       | F0       | FE       | Unknown D<br>Seconds          |
| 44  | 2C  | 39       | 39       | 39       | CC       | ^^^^
| 45  | 2D  | E0       | E0       | E0       | DF       | ^^^^
| 46  | 2E  | 02       | 02       | 02       | 02       | ^^^^
| 47  | 2F  | 1E       | 1E       | 1E       | 1E       | Unknown E<br>Counter          |
| 48  | 30  | 00       | 00       | 00       | 00       | ^^^^
| 49  | 31  | 00       | 00       | 00       | 00       | ^^^^
| 50  | 32  | 00       | 00       | 00       | 00       | ^^^^
| 51  | 33  | 00       | 00       | 00       | 00       | Summer Timer                  |
| 52  | 34  | 00       | 00       | 00       | 00       | Spa Loxk                      |
| 53  | 35  | 00       | 00       | 00       | 00       | Temp Lock                     |
| 54  | 36  | 00       | 00       | 00       | 00       | Clean Cycle                   |
| 55  | 37  | 00       | 00       | 00       | 00       | Jets2 Seconds                 |
| 56  | 38  | 00       | 00       | 00       | 00       | ^^^^
| 57  | 39  | 00       | 00       | 00       | 00       | ^^^^
| 58  | 3A  | 00       | 00       | 00       | 00       | ^^^^
| 59  | 3B  | 00       | 00       | 00       | 00       | Jets3 Seconds                 |
| 60  | 3C  | 00       | 00       | 00       | 00       | ^^^^
| 61  | 3D  | 00       | 00       | 00       | 00       | ^^^^
| 62  | 3E  | 00       | 00       | 00       | 00       | ^^^^
| 63  | 3F  | 00       | 00       | 00       | 00       | Unknown A<br>Seconds<br>Zero  |
| 64  | 40  | 00       | 00       | 00       | 00       | ^^^^
| 65  | 41  | 00       | 00       | 00       | 00       | ^^^^
| 66  | 42  | 00       | 00       | 00       | 00       | ^^^^
| 67  | 43  | 21       | 21       | 21       | 21       | Lights Runtime<br>Seconds     |
| 68  | 44  | A3       | A3       | A3       | A3       | ^^^^
| 69  | 45  | 06       | 06       | 06       | 06       | ^^^^
| 70  | 46  | 00       | 00       | 00       | 00       | ^^^^
| 71  | 47  | 02       | 02       | 02       | 02       | 1 Byte Value                  |
| 72  | 48  | 00       | 00       | 00       | 00       | Zero                          |
| 73  | 49  | B0       | B5       | BA       | C8       | Total Runtime Seconds         |
| 74  | 4A  | 38       | 38       | 38       | CB       | ^^^^
| 75  | 4B  | E0       | E0       | E0       | DF       | ^^^^
| 76  | 4C  | 02       | 02       | 02       | 02       | ^^^^
| 77  | 4D  | 00       | 00       | 00       | 00       | Jets1 Seconds A<br>Zero       |
| 78  | 4E  | 00       | 00       | 00       | 00       | ^^^^
| 79  | 4F  | 00       | 00       | 00       | 00       | ^^^^
| 80  | 50  | 00       | 00       | 00       | 00       | ^^^^
| 81  | 51  | 00       | 00       | 00       | 00       | Jets2 Seconds A<br>Zero       |
| 82  | 52  | 00       | 00       | 00       | 00       | ^^^^
| 83  | 53  | 00       | 00       | 00       | 00       | ^^^^
| 84  | 54  | 00       | 00       | 00       | 00       | ^^^^
| 85  | 55  | 31       | 31       | 31       | 31       | Set Temp                      |
| 86  | 56  | 30       | 30       | 30       | 30       | ^^^^
| 87  | 57  | 30       | 30       | 30       | 30       | ^^^^
| 88  | 58  | 46       | 46       | 46       | 46       | ^^^^
| 89  | 59  | 31       | 31       | 31       | 31       | Water Temp                    |
| 90  | 5A  | 30       | 30       | 30       | 30       | ^^^^
| 91  | 5B  | 33       | 33       | 33       | 30       | ^^^^
| 92  | 5C  | 46       | 46       | 46       | 46       | ^^^^
| 93  | 5D  | 78       | 78       | 78       | 76       | 2 byte value                  |
| 94  | 5E  | 00       | 00       | 00       | 00       | ^^^^
| 95  | 5F  | 05       | 05       | 05       | FC       | 2 byte value                  |
| 96  | 60  | 01       | 01       | 01       | 00       | ^^^^
| 97  | 61  | 05       | 05       | 05       | FC       | 2 byte value                  |
| 98  | 62  | 01       | 01       | 01       | 00       | ^^^^
| 99  | 63  | 00       | 00       | 00       | 00       | 2 byte value                  |
| 100 | 64  | 00       | 00       | 00       | 00       | ^^^^
| 101 | 65  | 00       | 00       | 00       | 00       | 2 byte value                  |
| 102 | 66  | 00       | 00       | 00       | 00       | ^^^^
| 103 | 67  | 00       | 00       | 00       | 00       | 2 byte value                  |
| 104 | 68  | 00       | 00       | 00       | 00       | ^^^^
| 105 | 69  | 00       | 00       | 00       | 00       | 2 byte value                  |
| 106 | 6A  | 00       | 00       | 00       | 00       | ^^^^
| 107 | 6B  | 00       | 00       | 00       | 00       | 1 Byte Shifted                |
| 108 | 6C  | 00       | 00       | 00       | 00       | Copy Previous                 |
| 109 | 6D  | 00       | 00       | 00       | 00       | 2 byte value                  |
| 110 | 6E  | 00       | 00       | 00       | 00       | ^^^^
| 111 | 6F  | 46       | 46       | 46       | 3E       | 2 byte value                  |
| 112 | 70  | 00       | 00       | 00       | 00       | ^^^^
| 113 | 71  | 00       | 00       | 00       | 00       | Heater Wattage                |
| 114 | 72  | 00       | 00       | 00       | 00       | ^^^^                          |
| 115 | 73  | 00       | 00       | 00       | 00       |                               |
| 116 | 74  | 00       | 00       | 00       | 00       |                               |
| 117 | 75  | 00       | 00       | 00       | 00       |                               |
| 118 | 76  | 3C       | 3C       | 3C       | 3C       | Constant 60                   |
| 119 | 77  | 00       | 00       | 00       | 00       |
| 120 | 78  | 1E       | 1E       | 1E       | 1E       | Constant 30                   |
| 121 | 79  | 00       | 00       | 00       | 00       |
| 122 | 7A  | 00       | 00       | 00       | 00       | 2 bits                        |
| 123 | 7B  | 75       | 75       | 75       | 6A       | 1 byte value                  |
| 124 | 7C  | 7C       | 56       | 73       | B6       | 2 byte value                  |
| 125 | 7D  | 04       | 04       | 04       | 01       | ^^^^                          |
| 126 | 7E  | 02       | 07       | 0C       | 13       | Seconds                       |
| 127 | 7F  | 08       | 08       | 08       | 17       | Minutes                       |
| 128 | 80  | 11       | 11       | 11       | 09       | Hours                         |
| 129 | 81  | 10       | 10       | 10       | 10       | Days                          |
| 130 | 82  | 08       | 08       | 08       | 08       | Months                        |
| 131 | 83  | D1       | D1       | D1       | D1       | Years                         |
| 132 | 84  | 07       | 07       | 07       | 07       | ^^^^
| 133 | 85  | 01       | 01       | 01       | 01       |                               |
