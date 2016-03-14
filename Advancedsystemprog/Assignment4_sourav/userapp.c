#include <linux/ioctl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define DEVICE "/dev/mycdrv"

#define CDRV_IOC_MAGIC 'Z'
#define ASP_CHGACCDIR _IOW(CDRV_IOC_MAGIC, 1, int)


int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Device number not specified\n");
		return 1;
	}
	int dev_no = atoi(argv[1]);
	char dev_path[20];
	int i,fd;
	char ch, write_buf[100], read_buf[100];
	int offset, origin;
	int dir;
	printf("devno =%d \n",dev_no);
	sprintf(dev_path, "%s%d", DEVICE, dev_no);
	printf( "dev_path %s\n", dev_path);
	fd = open(dev_path, O_RDWR);
	printf("fd=%d \n",fd);
	if(fd == -1) {
		printf("File %s either does not exist or has been locked by another "
				"process\n", dev_path);
		exit(-1);
	}
        while(1)
        {
	printf(" r = read from device after seeking to desired offset\n"
			" w = write to device \n");
	printf(" c = reverse direction of data access\n");
	printf(" o = change offset for data access\n");
	printf(" e = close and exit current device");
	printf("\n\n enter command :");
 
	scanf(" %c", &ch);
        
	switch(ch) {
	case 'w':
	
		printf("Enter Data to write: ");
		scanf(" %[^\n]", write_buf);
		write(fd, write_buf, strlen(write_buf));
		
				//lseek(fd, offset, strlen(write_buf));
		break;

	case 'c':
		printf(" 0 = regular \n 1 = reverse \n");
		printf(" \n enter direction :");
		scanf("%d", &dir);
		int rc = ioctl(fd, ASP_CHGACCDIR, &dir);
		if (rc == -1)
		{ perror("\n***error in ioctl***\n");
		return -1; }
		break;

	case 'r':
		printf("Origin \n 0 = beginning \n 1 = current \n 2 = end \n\n");
		printf(" enter origin :");
		scanf("%d", &origin);
		printf(" \n enter offset :");
		scanf("%d", &offset);
		lseek(fd, offset, origin);
		if (read(fd, read_buf, sizeof(read_buf)) > 0) {
			printf("\ndevice: %s\n", read_buf);
		} else {
			fprintf(stderr, "Reading failed\n");
		}
		
		break;
	case 'o': 
		printf(" \n enter offset :");
		scanf("%d", &offset);
		lseek(fd, offset, 0);
		break;	
	case 'e':
		printf("Exiting and closing open device %s\n",dev_path);
		close(fd);
		return 0;
	default:
		printf("Command not recognized\n");
		break;

	}
	fseek(stdin,0,SEEK_END);
	}

}
