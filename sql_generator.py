
name = 'test'

def print_insert(size):
	for i in range(size):
		print('insert into '+name+' values('+str(i)+',\''+str(i)+'\','+str(i)+');')

print_insert(10000)