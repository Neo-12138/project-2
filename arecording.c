#include "arecording.h"

int init_sock(void)
{
    // 创建套接字
    int soc_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == soc_fd) {
        perror("socket failed!\n");
        exit(-1);
    }
    // 设置端口复用
    int on = 1;
    setsockopt(soc_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    // 发起连接请求
    printf("正在连接\n");

    socklen_t len = sizeof(struct sockaddr);
    // 配置结构体
    struct sockaddr_in jack;
    jack.sin_family      = AF_INET;
    jack.sin_port        = htons(54321);
    jack.sin_addr.s_addr = inet_addr("192.168.11.234");

    int con_fd = connect(soc_fd, (struct sockaddr *)&jack, len);
    if(-1 == con_fd) {
        perror("connect failed!\n");
        exit(-1);
    }
    printf("已连接！\n");
    return soc_fd;
}

// 发送录音文件
int send_file(int fd)
{
    int wav_fd = open("./example.wav", O_RDWR);
    if(-1 == wav_fd) {
        perror("open wav failed!\n");
        return -1;
    }
    char buf[1024];
    int sum = 0;
    int ret;
    while(1) {
        memset(buf, 0, sizeof(buf));
        ret = read(wav_fd, buf, 1024);
        if(-1 == ret) {
            perror("read wav failed!\n");
            return -1;
        } else if(0 == ret)
            break;
        else {
            sum += ret;
            write(fd, buf, ret);
        }
    }
    printf("写入了%d个字节", sum);
    close(wav_fd);
    printf("传输成功！\n");
    return 0;
}

int rec_file(int fd)
{
    char xml_buf[1024];
    int xml_fd;
    xml_fd = open("./result.xml", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if(-1 == xml_fd) {
        perror("open result failed!\n");
        return -1;
    }
    int ret = read(fd, xml_buf, 1024);
    write(xml_fd, xml_buf, ret);
    close(xml_fd);
    return 0;
}
// 传入的cur == object作为根节点
xmlChar * __get_cmd_id(xmlDocPtr doc, xmlNodePtr cur)
{
    xmlChar *key, *id;

    cur = cur->xmlChildrenNode;

    while(cur != NULL) {
        // 查找到cmd子节点
        if((!xmlStrcmp(cur->name, (const xmlChar *)"cmd"))) {
            // 取出内容
            key = xmlNodeGetContent(cur);

            printf("cmd: %s\n", key);
            xmlFree(key);

            // 读取节点属性
            id = xmlGetProp(cur, (const xmlChar *)"id");
            printf("id: %s\n", id);

            xmlFree(doc);
            return id;
        }
        cur = cur->next;
    }
    // 释放文档指针
    xmlFree(doc);
    return NULL;
}

xmlChar * parse_xml(char * xmlfile)
{
    xmlDocPtr doc;
    xmlNodePtr cur1, cur2;
    // 分析一个xml文件，并返回一个xml文档的对象指针： 也就是指向树
    doc = xmlParseFile(xmlfile);
    if(doc == NULL) {
        fprintf(stderr, "Document not parsed successfully. \n");
        return NULL;
    }
    // 获得文档的根节点
    cur1 = xmlDocGetRootElement(doc);
    if(cur1 == NULL) {
        fprintf(stderr, "empty document\n");
        xmlFreeDoc(doc);
        return NULL;
    }
    // 检查根节点的名称是否为nlp
    if(xmlStrcmp(cur1->name, (const xmlChar *)"nlp")) {
        fprintf(stderr, "document of the wrong type, root node != nlp");
        xmlFreeDoc(doc);
        return NULL;
    }
    // 获取子元素节点
    cur1 = cur1->xmlChildrenNode;

    while(cur1 != NULL) {
        // 检查子元素是否为result
        if((!xmlStrcmp(cur1->name, (const xmlChar *)"result"))) {
            // 得到result的子节点
            cur2 = cur1->xmlChildrenNode;
            while(cur2 != NULL) {
                // 查找到准确率
                if((!xmlStrcmp(cur2->name, (const xmlChar *)"confidence"))) {
                    xmlChar * key = xmlNodeGetContent(cur2);
                    printf("confidence: %s\n", key);

                    // 若准确率低于30，则放弃当前识别
                    if(atoi((char *)key) < 30) {
                        xmlFree(doc);
                        fprintf(stderr, "sorry, I'm NOT sure what you say.\n");
                        return NULL;
                    }
                }
                // 查找到object，则执行提取字符串及属性
                if((!xmlStrcmp(cur2->name, (const xmlChar *)"object"))) {
                    return __get_cmd_id(doc, cur2);
                }
                cur2 = cur2->next;
            }
        }
        cur1 = cur1->next;
    }

    // 释放文档指针
    xmlFreeDoc(doc);
    return NULL;
}
