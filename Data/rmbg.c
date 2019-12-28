#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>


#define PNG_BYTES_TO_CHECK  8
#define HAVE_ALPHA          1
#define NOT_HAVE_ALPHA      0

typedef struct _pic_data pic_data;
struct _pic_data {
    int width, height;  //长宽
    int bit_depth;      //位深度
    int alpha_flag;     //是否有透明通道
    unsigned char *rgba;//实际rgb数据
};

int check_is_png(FILE **fp, const char *filename) //检查是否png文件
{
    char checkheader[PNG_BYTES_TO_CHECK]; //查询是否png头
    *fp = fopen(filename, "rb");
    if (*fp == NULL) {
        printf("open failed ...1\n");
        return -1;
    }
    if (fread(checkheader, 1, PNG_BYTES_TO_CHECK, *fp) != PNG_BYTES_TO_CHECK) //读取png文件长度错误直接退出
        return 0;
    return png_sig_cmp(checkheader, 0, PNG_BYTES_TO_CHECK); //0正确, 非0错误
}

int decode_png(const char *filename, pic_data *out) //取出png文件中的rgb数据
{
    png_structp png_ptr; //png文件句柄
    png_infop   info_ptr;//png图像信息句柄
    int ret;
    FILE *fp;
    if (check_is_png(&fp, filename) != 0) {
        printf("file is not png ...\n");
        return -1;
    }
    printf("launcher[%s] ...\n", PNG_LIBPNG_VER_STRING); //打印当前libpng版本号

    //1: 初始化libpng的数据结构 :png_ptr, info_ptr
    png_ptr  = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    info_ptr = png_create_info_struct(png_ptr);

    //2: 设置错误的返回点
    setjmp(png_jmpbuf(png_ptr));
    rewind(fp); //等价fseek(fp, 0, SEEK_SET);

    //3: 把png结构体和文件流io进行绑定
    png_init_io(png_ptr, fp);
    //4:读取png文件信息以及强转转换成RGBA:8888数据格式
    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND, 0); //读取文件信息
    int channels, color_type;
    channels    = png_get_channels(png_ptr, info_ptr); //通道数量
    color_type  = png_get_color_type(png_ptr, info_ptr);//颜色类型
    out->bit_depth = png_get_bit_depth(png_ptr, info_ptr);//位深度
    out->width   = png_get_image_width(png_ptr, info_ptr);//宽
    out->height  = png_get_image_height(png_ptr, info_ptr);//高

    //if(color_type == PNG_COLOR_TYPE_PALETTE)
    //  png_set_palette_to_rgb(png_ptr);//要求转换索引颜色到RGB
    //if(color_type == PNG_COLOR_TYPE_GRAY && out->bit_depth < 8)
    //  png_set_expand_gray_1_2_4_to_8(png_ptr);//要求位深度强制8bit
    //if(out->bit_depth == 16)
    //  png_set_strip_16(png_ptr);//要求位深度强制8bit
    //if(png_get_valid(png_ptr,info_ptr,PNG_INFO_tRNS))
    //  png_set_tRNS_to_alpha(png_ptr);
    //if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    //  png_set_gray_to_rgb(png_ptr);//灰度必须转换成RG
    printf("channels = %d color_type = %d bit_depth = %d width = %d height = %d ...\n",
            channels, color_type, out->bit_depth, out->width, out->height);

    int i, j, k;
    int size, pos = 0;
    int temp;

    //5: 读取实际的rgb数据
    png_bytepp row_pointers; //实际存储rgb数据的buf
    row_pointers = png_get_rows(png_ptr, info_ptr); //也可以分别每一行获取png_get_rowbytes();
    size = out->width * out->height; //申请内存先计算空间
    if (channels == 4 || color_type == PNG_COLOR_TYPE_RGB_ALPHA) { //判断是24位还是32位
        out->alpha_flag = HAVE_ALPHA; //记录是否有透明通道
        size *= (sizeof(unsigned char) * 4); //size = out->width * out->height * channel
        out->rgba = (png_bytep)malloc(size);
        if (NULL == out->rgba) {
            printf("malloc rgba faile ...\n");
            png_destroy_read_struct(&png_ptr, &info_ptr, 0);
            fclose(fp);
            return -1;
        }
        //从row_pointers里读出实际的rgb数据出来
        temp = channels - 1;
        for (i = 0; i < out->height; i++)
            for (j = 0; j < out->width * 4; j += 4)
                for (k = temp; k >= 0; k--)
                    out->rgba[pos++] = row_pointers[i][j + k];
    } else if (channels == 3 || color_type == PNG_COLOR_TYPE_RGB) { //判断颜色深度是24位还是32位
        out->alpha_flag = NOT_HAVE_ALPHA;
        size *= (sizeof(unsigned char) * 3);
        out->rgba = (png_bytep)malloc(size);
        if (NULL == out->rgba) {
            printf("malloc rgba faile ...\n");
            png_destroy_read_struct(&png_ptr, &info_ptr, 0);
            fclose(fp);
            return -1;
        }
        //从row_pointers里读出实际的rgb数据
        temp = (3 * out->width);
        for (i = 0; i < out->height; i ++) {
            for (j = 0; j < temp; j += 3) {
                out->rgba[pos++] = row_pointers[i][j+2];
                out->rgba[pos++] = row_pointers[i][j+1];
                out->rgba[pos++] = row_pointers[i][j+0];
            }
        }
    } else return -1;
    //6:销毁内存
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
    fclose(fp);
    //此时， 我们的out->rgba里面已经存储有实际的rgb数据了
    //处理完成以后free(out->rgba)
    return 0;
}

int RotationRight90_1(pic_data *in) //顺时针旋转90度
{
    unsigned char * tempSrc = NULL; //临时的buf用来记录原始的图像(未旋转之前的图像)
    int mSize = 0, i = 0, j = 0, k = 0, channel = 0;
    int desW = 0;
    int desH = 0;
    int tmp1, tmp2;

    desW = in->height;
    desH = in->width;
    if(in->alpha_flag == HAVE_ALPHA)
    {
        channel = 4;
    }
    else
    {
        channel = 3;
    }
    mSize = (in->width) * (in->height) * sizeof(char) * (channel);
 //   printf("start copy\n");
    tempSrc = (unsigned char *)malloc(mSize);
    memcpy(tempSrc, in->rgba, mSize); //拷贝原始图像至tempbuf
//    printf("copy ok--------\n");
    for(i = 0; i < desH; i ++)
    {
        for(j = 0; j < desW; j ++)
        {
            for(k = 0; k < channel; k ++)
            {
                tmp1 = (i * desW + j) * channel + k;
                tmp2 = (((in->height) - 1 - j) * (in->width) + i) * channel + k;
                in->rgba[(i * desW + j) * channel + k] = tempSrc[(((in->height) - 1 - j) * (in->width) + i) * channel + k]; //替换像素
//                printf("tmp1 = %d, tmp2 = %d\n", tmp1, tmp2);
            }
        }
    }
    free(tempSrc);
    in->height = desH;
    in->width = desW;
    return 0;
}

// 统一格式，将png转换成具有ALPHA模式
int convert_to_alphamode(pic_data *in)      
{
    unsigned char * tempSrc = NULL;         // 临时的buf用来记录原始的图像(未旋转之前的图像)
    int temp;
    int i, j, pos = 0, pos_out = 0;
    unsigned char *pchar;

    if (in->alpha_flag == HAVE_ALPHA)
    {
        temp = (4 * in->width);
        printf("already have alpha ...\n");
        return 1;
    }
    else
    {
        in->alpha_flag = HAVE_ALPHA;
        temp = (3 * in->width);
        printf("not have alpha ...\n");
    }
    pchar = in->rgba;
    in->rgba = (unsigned char *)malloc(sizeof(char) * (in->width) * (in->height) * 4);

    for (i = 0; i < in->height; i++)
    {
        for (j = 0; j < temp; j += 3)
        {
            in->rgba[pos_out++] = 0xFF;
            in->rgba[pos_out++] = pchar[pos++];
            in->rgba[pos_out++] = pchar[pos++];
            in->rgba[pos_out++] = pchar[pos++];
        }
    }
    free(pchar);
    printf("change to alpha ok\n");
    return 2;
}

// 将黑色设置成透明
int png_set_alpha(pic_data *in)
{
    int temp;
    int i, j, pos = 0;
    if (in->alpha_flag != HAVE_ALPHA)
    {
        printf("png_set_alpha fail, change png to alpha mode first...\n");
        return -1;
    }
    temp = (4 * in->width);
    for (i = 0; i < in->height; i++)
    {
        pos = i * (in->width) * 4;
        for (j = 0; j < temp; j += 4)
        {
            if ((in->rgba[pos + j + 1] + in->rgba[pos + j + 2] + in->rgba[pos + j + 3]) < 60)       // 黑色区域都变成透明的
            {
                //printf("i = %d, j = %d, pos = %d\n", i, j, pos);
                in->rgba[pos + j + 0] = 0x00;
            }
            else        // 将有颜色的都变成白色
            {
                in->rgba[pos + j + 0] = 0xFF;
                in->rgba[pos + j + 1] = 0xFF;
                in->rgba[pos + j + 2] = 0xFF;
                in->rgba[pos + j + 3] = 0xFF;
            }
        }
    }
    printf("png_set_alpha ok\n");
    return 1;
}

// 将白色设置成透明
int png_set_alpha_1(pic_data *in)
{
    int temp;
    int i, j, pos = 0;
    if (in->alpha_flag != HAVE_ALPHA)
    {
        printf("png_set_alpha fail, change png to alpha mode first...\n");
        return -1;
    }
    temp = (4 * in->width);
    for (i = 0; i < in->height; i++)
    {
        pos = i * (in->width) * 4;
        for (j = 0; j < temp; j += 4)
        {
            if ((in->rgba[pos + j + 1] + in->rgba[pos + j + 2] + in->rgba[pos + j + 3]) > 600)       // 白色区域都变成透明的
            {
                //printf("i = %d, j = %d, pos = %d\n", i, j, pos);
                in->rgba[pos + j + 0] = 0x00;       // 透明
            }
            else        // 将有颜色的都变成白色
            {
                in->rgba[pos + j + 0] = 0xFF;       // 非透明
                in->rgba[pos + j + 1] = 0x00;
                in->rgba[pos + j + 2] = 0x00;
                in->rgba[pos + j + 3] = 0x00;
            }
        }
    }
    printf("png_set_alpha ok\n");
    return 1;
}

// 截取图片
// 参数：
// startx: 新图片的起始x坐标
// starty: 新图片的起始y坐标
// newwidth: 新图片的宽度
// newheight: 新图片的高度
int screen_cut(pic_data *in, int startx, int starty, int newwidth, int newheight)
{
    int temp = 0;
    int channel = 0;
    int i, j, k, pos = 0;
    unsigned char *puchar;
    if ((newwidth > in->width) || (newheight > in->height))
    {
        printf("error: newwidth or newheight error, -1\n");
        return -1;
    }
    if (((startx + newwidth) > in->width) || ((starty + newheight) > in->height))
    {
        printf("error: startx + newwidth or starty + newheight error, -2\n");
        return -2;
    }
    
    if((startx < 0) || (starty < 0) || (newwidth < 1) || (newheight < 1))
    {
        printf("error: parameter error, -3\n");
        return -3;
    }
    
    if (in->alpha_flag == HAVE_ALPHA)
    {
        channel = 4;
    }
    else
    {
        channel = 3;
    }
    temp = newwidth * newheight * sizeof(char) * channel;
    puchar = in->rgba;
    in->rgba = (unsigned char *)malloc(temp);
    for(i = starty; i < (newheight + starty); i++)
    {
        temp = i * (in->width) * channel;
        for(j = startx; j < (newwidth + startx); j++)
        {
            for(k = 0; k < channel; k++)
            {
                in->rgba[((i - starty) * newwidth + (j - startx)) * channel + k] = puchar[temp + j*channel + k];
            }
        }
    }
    in->width = newwidth;
    in->height = newheight;
    free(puchar);
    return 1;
}

int write_png_file(const char *filename , pic_data *out) //生成一个新的png图像
{
    png_structp png_ptr;
    png_infop   info_ptr;
    png_byte color_type;
    png_bytep * row_pointers;
    FILE *fp = fopen(filename, "wb");
    if (NULL == fp) {
        printf("open failed ...2\n");
        return -1;
    }
    //1: 初始化libpng结构体
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (!png_ptr) {
        printf("png_create_write_struct failed ...\n");
        return -1;
    }
    //2: 初始化png_infop结构体 ，
    //此结构体包含了图像的各种信息如尺寸，像素位深, 颜色类型等等
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        printf("png_create_info_struct failed ...\n");
        return -1;
    }
    //3: 设置错误返回点
    if (setjmp(png_jmpbuf(png_ptr))) {
        printf("error during init_io ...\n");
        return -1;
    }
    //4:绑定文件IO到Png结构体
    png_init_io(png_ptr, fp);
    if (setjmp(png_jmpbuf(png_ptr))) {
        printf("error during init_io ...\n");
        return -1;
    }
    if (out->alpha_flag == HAVE_ALPHA) color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    else color_type = PNG_COLOR_TYPE_RGB;
    //5：设置以及写入头部信息到Png文件
    png_set_IHDR(png_ptr, info_ptr, out->width, out->height, out->bit_depth,
    color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png_ptr, info_ptr);
    if (setjmp(png_jmpbuf(png_ptr))) {
        printf("error during init_io ...\n");
        return -1;
    }
    int channels, temp;
    int i, j, pos = 0;
    if (out->alpha_flag == HAVE_ALPHA) {
        channels = 4;
        temp = (4 * out->width);
        printf("have alpha ...\n");
    } else {
        channels = 3;
        temp = (3 * out->width);
        printf("not have alpha ...\n");
    }
    // 顺时针旋转90度 ， 旋转完了一定要把width 和height调换 不然得到的图像是花的  旋转三次就是逆时针旋转一次
    //RotationRight90(out->rgba, &out->width, &out->height, channels);
    //RotationRight90(out->rgba, out->height, out->width, channels);
    //RotationRight90(out->rgba, out->width, out->height, channels);
    row_pointers = (png_bytep*)malloc(out->height * sizeof(png_bytep));
    for (i = 0; i < out->height; i++) {
        row_pointers[i] = (png_bytep)malloc(temp* sizeof(unsigned char));
        for (j = 0; j < temp; j += channels) {
            if (channels == 4) {
                //if (out->alpha_flag == HAVE_ALPHA)
                row_pointers[i][j+3] = out->rgba[pos++];
                row_pointers[i][j+2] = out->rgba[pos++];
                row_pointers[i][j+1] = out->rgba[pos++];
                row_pointers[i][j+0] = out->rgba[pos++];
                //printf("----    i = %d, j = %d, %d, %d, %d, %d\n", i, j, row_pointers[i][j + 0], row_pointers[i][j + 1],row_pointers[i][j + 2],row_pointers[i][j + 3]);
                //printf("----    i = %d, j = %d, %d, %d, %d, %d\n", i, j, row_pointers[i][j + 0], row_pointers[i][j + 1],row_pointers[i][j + 2],row_pointers[i][j + 3]);
            } else {
                row_pointers[i][j+2] = out->rgba[pos++];
                row_pointers[i][j+1] = out->rgba[pos++];
                row_pointers[i][j+0] = out->rgba[pos++];
                //printf("++++    i = %d, j = %d, %d, %d, %d, %d\n", i, j, row_pointers[i][j + 0], row_pointers[i][j + 1],row_pointers[i][j + 2],row_pointers[i][j + 3]);
            }
        }
    }
    //6: 写入rgb数据到Png文件
    png_write_image(png_ptr, (png_bytepp)row_pointers);
    if (setjmp(png_jmpbuf(png_ptr))) {
        printf("error during init_io ...\n");
        return -1;
    }
    //7: 写入尾部信息
    png_write_end(png_ptr, NULL);
    //8:释放内存 ,销毁png结构体
    for (i = 0; i < out->height; i ++)
        free(row_pointers[i]);
    free(row_pointers);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    return 0;
}

int main(int argc, char **argv)
{
    pic_data out;
    
    if(argc == 2)
    {
        if(strstr(argv[1],"help") != NULL)
        {
            printf("\n===============================================================================\n");
            printf("    usage: %s -cmd parameter...\n", argv[0]);
            printf("    cmd1(change to alpha mode): %s 1 srcfilename outfilename\n", argv[0]);
            printf("    cmd2(screen cut): %s 2 srcfilename outfilename startx starty newwidth newheight\n", argv[0]);
            printf("    cmd3(change black to transparent): %s 3 srcfilename outfilename\n", argv[0]);
            printf("    cmd4(change write to transparent): %s 4 srcfilename outfilename\n", argv[0]);
            printf("===============================================================================\n\n");
            
            return 0;
        }
    }
    if(argc >= 4)
    {
        if(strcmp(argv[1],"1") == 0)        // cmd1
        {
            printf("    usage: %s 1 in.png out.png\n", argv[0]);
            decode_png(argv[2], &out);
            convert_to_alphamode(&out);
            write_png_file(argv[3], &out);
            free(out.rgba);
            return 1;
        }
        else if(strcmp(argv[1],"2") == 0)        // cmd2
        {
            printf("    usage: %s 2 in.png out.png startx starty newwidth newheight\n", argv[0]);
            printf("    startx = %d\n", atoi(argv[4]));
            printf("    starty = %d\n", atoi(argv[5]));
            printf("    newwidth = %d\n", atoi(argv[6]));
            printf("    newheight = %d\n", atoi(argv[7]));

            if(argc != 8)
            {
                printf("Error: parameter count not correct\n");
                return -1;
            }
            decode_png(argv[2], &out);
            screen_cut(&out, atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]));
            write_png_file(argv[3], &out);
        }
        else if(strcmp(argv[1],"3") == 0)        // cmd3
        {
            printf("    usage: %s 3 in.png out.png\n", argv[0]);
            decode_png(argv[2], &out);
            png_set_alpha(&out);
            write_png_file(argv[3], &out);
            free(out.rgba);
        }
        else if(strcmp(argv[1],"4") == 0)        // cmd4
        {
            printf("    usage: %s 4 in.png out.png\n", argv[0]);
            decode_png(argv[2], &out);
            png_set_alpha_1(&out);
            write_png_file(argv[3], &out);
            free(out.rgba);
        }
    }
    return 0;
}
