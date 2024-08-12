#include "request_gy39.h"

int init_tty(int fd)
{
    // 1.获取串口原先配置
    struct termios old_cfg;
    memset(&old_cfg, 0, sizeof(old_cfg));
    if(tcgetattr(fd, &old_cfg) != 0) {
        perror("tcgetattr");
        return -1;
    }
    // 2.定义新的存放属性的结构体变量
    struct termios new_cfg;
    memset(&new_cfg, 0, sizeof(new_cfg));
    // 激活本地连接和接收使能
    new_cfg.c_cflag |= CLOCAL | CREAD;
    // 设置终端工作模式为原始模式
    cfmakeraw(&new_cfg);
    // 设置输入输出波特率
    cfsetispeed(&new_cfg, B9600);
    cfsetospeed(&new_cfg, B9600);
    // 设置数据位为8位
    new_cfg.c_cflag &= ~CSIZE; /* 用数据位掩码清空数据位设置 */
    new_cfg.c_cflag |= CS8;
    // 设置无奇偶校验
    new_cfg.c_cflag &= ~PARENB;
    // 设置停止位
    new_cfg.c_cflag &= ~CSTOPB; /* 将停止位设置为一个比特 */
    // 设置等待时间和最少接收数据
    new_cfg.c_cc[VTIME] = 0; // 一直阻塞
    new_cfg.c_cc[VMIN]  = 1; // 有数据就马上接收
    // 用于清空输入/输出缓冲区
    tcflush(fd, TCIOFLUSH);
    // 设置立即生效
    if((tcsetattr(fd, TCSANOW, &new_cfg)) != 0) {
        perror("tcsetattr");
        return -1;
    }

    return 0;
}

gy_info * request_gy39()
{
    // 1.打开串口2
    int s_fd2 = open("/dev/ttySAC2", O_RDWR | O_NOCTTY);
    if(s_fd2 == -1) {
        perror("open sac2 error\n");
        return NULL;
    }
    // 2.设置串口属性
    init_tty(s_fd2);

    // 发送指令，读取所有的数据
    unsigned char cmd[3] = {0xA5, 0x83, 0x28};
    int w                = write(s_fd2, cmd, 3);
    if(w == -1) {
        perror("write error\n");
        return NULL;
    }
    sleep(1);

    unsigned char cmd1[3] = {0xA5, 0xAE, 0x53};
    w                     = write(s_fd2, cmd1, 3);
    if(w == -1) {
        perror("write error\n");
        return NULL;
    }
    sleep(1);

    // 3.读取数据
    unsigned char buf[24] = {0};
    gy_info * info        = (gy_info *)calloc(1, sizeof(gy_info)); // 存读取到的数据
    int ret;

    while(1) {
        memset(buf, 0, 24);
        ret = read(s_fd2, buf, 24);
        for(int i = 0; i < 24; i++) printf("%x ", buf[i]);

        // 第一个数据帧和第二个数据帧没有拿到光照和温度
        if(ret == -1 || buf[0] != 0x5A || buf[1] != 0x5A || buf[2] != 0x15 || buf[9] != 0x5A || buf[10] != 0x5A ||
           buf[11] != 0x45) {
            sleep(1);
            printf("read error\n");
            printf("ret = %d\n", ret);
            printf("数据错误：\n");
            for(int i = 0; i < 24; i++) printf("%x ", buf[i]);
            continue;
        }

        // 读取成功
        // 写入输出光照强度指令
        unsigned char cmd[3] = {0xA5, 0x51, 0xF6};
        int w_ret            = write(s_fd2, cmd, 3);
        if(w_ret == -1) {
            perror("write error\n");
            return -1;
        }
        sleep(1);
        // 获取光照
        if(buf[2] == 0x15) {
            double lux = buf[4] << 24 | buf[5] << 16 | buf[6] << 8 | buf[7] << 0;
            lux /= 100;
            printf("光强=%.2lf\n", lux);
            info->lux = lux;
        }
        unsigned char cmd1[3] = {0xA5, 0x52, 0xF7};
        w_ret                 = write(s_fd2, cmd1, 3);
        if(w_ret == -1) {
            perror("write error\n");
            return -1;
        }
        sleep(1);
        if(buf[11] == 0x45) {
            // 获取温度
            double T = buf[13] << 8 | buf[14] << 0;
            T /= 100;
            printf("温度=%.2lf\n", T);
            info->tem = T;

            // 获取气压
            double p = buf[15] << 24 | buf[16] << 16 | buf[17] << 8 | buf[18] << 0;
            p /= 100;
            printf("气压=%.2lf\n", p);
            info->atm = p;

            // 获取湿度
            double Hum = buf[19] << 8 | buf[20] << 0;
            Hum /= 100;
            printf("湿度=%.2lf%%\n", Hum);
            info->hum = Hum;

            // 获取海拔
            double H = buf[21] << 8 | buf[22] << 0;
            printf("海拔=%.2lf\n", H);
            info->alt = H;
        }
        break;
    }
    close(s_fd2);
    return info;
}