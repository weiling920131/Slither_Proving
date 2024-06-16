file = open('test.txt', 'r')
cnt = 0
for line in file:
    cnt += 1
print(cnt)