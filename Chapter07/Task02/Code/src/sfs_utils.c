#include "../include/sfs.h"

extern struct sfs_super      sfs_super; 
extern struct custom_options sfs_options;

/**
 * @brief 获取文件名
 * 
 * @param path 
 * @return char* 
 */
char* sfs_get_fname(const char* path) {
    char ch = '/';
    char *q = strrchr(path, ch) + 1;
    return q;
}
/**
 * @brief 计算路径的层级
 * exm: /av/c/d/f
 * -> lvl = 4
 * @param path 
 * @return int 
 */
int sfs_calc_lvl(const char * path) {
    // char* path_cpy = (char *)malloc(strlen(path));
    // strcpy(path_cpy, path);
    char* str = path;
    int   lvl = 0;
    if (strcmp(path, "/") == 0) {
        return lvl;
    }
    while (*str != NULL) {
        if (*str == '/') {
            lvl++;
        }
        str++;
    }
    return lvl;
}
/**
 * @brief 驱动读
 * 
 * @param offset 
 * @param out_content 
 * @param size 
 * @return int 
 */
int sfs_driver_read(int offset, uint8_t *out_content, int size) {
    int      offset_aligned = SFS_ROUND_DOWN(offset, SFS_IO_SZ());
    int      bias           = offset - offset_aligned;
    int      size_aligned   = SFS_ROUND_UP((size + bias), SFS_IO_SZ());
    uint8_t* temp_content   = (uint8_t*)malloc(size_aligned);
    uint8_t* cur            = temp_content;
    // lseek(SFS_DRIVER(), offset_aligned, SEEK_SET);
    ddriver_seek(SFS_DRIVER(), offset_aligned, SEEK_SET);
    while (size_aligned != 0)
    {
        // read(SFS_DRIVER(), cur, SFS_IO_SZ());
        ddriver_read(SFS_DRIVER(), cur, SFS_IO_SZ());
        cur          += SFS_IO_SZ();
        size_aligned -= SFS_IO_SZ();   
    }
    memcpy(out_content, temp_content + bias, size);
    free(temp_content);
    return SFS_ERROR_NONE;
}
/**
 * @brief 驱动写
 * 
 * @param offset 
 * @param in_content 
 * @param size 
 * @return int 
 */
int sfs_driver_write(int offset, uint8_t *in_content, int size) {
    int      offset_aligned = SFS_ROUND_DOWN(offset, SFS_IO_SZ());
    int      bias           = offset - offset_aligned;
    int      size_aligned   = SFS_ROUND_UP((size + bias), SFS_IO_SZ());
    uint8_t* temp_content   = (uint8_t*)malloc(size_aligned);
    uint8_t* cur            = temp_content;
    sfs_driver_read(offset_aligned, temp_content, size_aligned);
    memcpy(temp_content + bias, in_content, size);
    
    // lseek(SFS_DRIVER(), offset_aligned, SEEK_SET);
    ddriver_seek(SFS_DRIVER(), offset_aligned, SEEK_SET);
    while (size_aligned != 0)
    {
        // write(SFS_DRIVER(), cur, SFS_IO_SZ());
        ddriver_write(SFS_DRIVER(), cur, SFS_IO_SZ());
        cur          += SFS_IO_SZ();
        size_aligned -= SFS_IO_SZ();   
    }

    free(temp_content);
    return SFS_ERROR_NONE;
}
/**
 * @brief 为一个inode分配dentry，采用头插法
 * 
 * @param inode 
 * @param dentry 
 * @return int 
 */
int sfs_alloc_dentry(struct sfs_inode* inode, struct sfs_dentry* dentry) {
    if (inode->dentrys == NULL) {
        inode->dentrys = dentry;
    }
    else {
        dentry->brother = inode->dentrys;
        inode->dentrys = dentry;
    }
    inode->dir_cnt++;
    return inode->dir_cnt;
}
/**
 * @brief 将dentry从inode的dentrys中取出
 * 
 * @param inode 
 * @param dentry 
 * @return int 
 */
int sfs_drop_dentry(struct sfs_inode * inode, struct sfs_dentry * dentry) {
    boolean is_find = FALSE;
    struct sfs_dentry* dentry_cursor;
    dentry_cursor = inode->dentrys;
    
    if (dentry_cursor == dentry) {
        inode->dentrys = dentry->brother;
        is_find = TRUE;
    }
    else {
        while (dentry_cursor)
        {
            if (dentry_cursor->brother == dentry) {
                dentry_cursor->brother = dentry->brother;
                is_find = TRUE;
                break;
            }
            dentry_cursor = dentry_cursor->brother;
        }
    }
    if (!is_find) {
        return -SFS_ERROR_NOTFOUND;
    }
    inode->dir_cnt--;
    return inode->dir_cnt;
}
/**
 * @brief 分配一个inode，占用位图
 * 
 * @param dentry 该dentry指向分配的inode
 * @return sfs_inode
 */
struct sfs_inode* sfs_alloc_inode(struct sfs_dentry * dentry) {
    struct sfs_inode* inode;
    int byte_cursor = 0; 
    int bit_cursor  = 0; 
    int ino_cursor  = 0;
    int dno_cursor  = 0;
    boolean is_find_free_entry = FALSE;
       
    // 寻找一个空闲的 Inode
    for (byte_cursor = 0; byte_cursor < SFS_BLKS_SZ(sfs_super.map_inode_blks); 
         byte_cursor++)
    {
        
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            if((sfs_super.map_inode[byte_cursor] & (0x1 << bit_cursor)) == 0) {    
                                                      /* 当前ino_cursor位置空闲 */
                SFS_DBG("Alloc Inode Map byte_cursor[%d] bit_cursor[%d]\n", byte_cursor,0x1 << bit_cursor);
                sfs_super.map_inode[byte_cursor] |= (0x1 << bit_cursor);
                is_find_free_entry = TRUE;           
                break;
            }
            ino_cursor++;
        }
        if (is_find_free_entry) {
            break;
        }
    }

    if (!is_find_free_entry || ino_cursor == sfs_super.max_ino)
        return -SFS_ERROR_NOSPACE;
   
    inode = (struct sfs_inode*)malloc(sizeof(struct sfs_inode));
    inode->ino  = ino_cursor; 
    inode->size = 0;
                                                      /* dentry指向inode */
    dentry->inode = inode;
    dentry->ino   = inode->ino;
                                                      /* inode指回dentry */
    inode->dentry = dentry;
    
    inode->dir_cnt = 0;
    inode->dentrys = NULL;
    
    // 分配数据块，普通文件用于保存文件内容，目录用于保存目录下面的文件名

    is_find_free_entry = FALSE;
    // 我们寻找一个空闲的 数据块（预先分配磁盘空间，这样可以简化操作）
    for (byte_cursor = 0; byte_cursor < SFS_BLKS_SZ(sfs_super.map_data_blks); 
        byte_cursor++)
    {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            if((sfs_super.map_data[byte_cursor] & (0x1 << bit_cursor)) == 0) {    
                                                    /* 当前ino_cursor位置空闲 */
                SFS_DBG("Alloc Data Map byte_cursor[%d] bit_cursor[%d]\n", byte_cursor,0x1 << bit_cursor);
                sfs_super.map_data[byte_cursor] |= (0x1 << bit_cursor);
                is_find_free_entry = TRUE;           
                break;
            }
            dno_cursor++;
        }
        if (is_find_free_entry) {
            break;
        }
    }

    if (!is_find_free_entry)
        return -SFS_ERROR_NOSPACE;  

    // 记录分配的磁盘数据号
    inode->dno = dno_cursor;

    if (SFS_IS_REG(inode)) {
        inode->data = (uint8_t *)malloc(SFS_BLKS_SZ(SFS_DATA_PER_FILE));
    }
    
    return inode;
}
/**
 * @brief 将内存inode及其下方结构全部刷回磁盘
 * 
 * @param inode 
 * @return int 
 */
int sfs_sync_inode(struct sfs_inode * inode) {
    struct sfs_inode_d  inode_d;
    struct sfs_dentry*  dentry_cursor;
    struct sfs_dentry_d dentry_d;
    int ino             = inode->ino;
    inode_d.ino         = ino;
    int dno             = inode->dno;
    inode_d.dno         = dno;
    inode_d.size        = inode->size;
    memcpy(inode_d.target_path, inode->target_path, SFS_MAX_FILE_NAME);
    inode_d.ftype       = inode->dentry->ftype;
    inode_d.dir_cnt     = inode->dir_cnt;
    int offset;
    
    if (sfs_driver_write(SFS_INO_OFS(ino), (uint8_t *)&inode_d, 
                     sizeof(struct sfs_inode_d)) != SFS_ERROR_NONE) {
        SFS_DBG("[%s] io error\n", __func__);
        return -SFS_ERROR_IO;
    }
                                                      /* Cycle 1: 写 INODE */
                                                      /* Cycle 2: 写 数据 */
    if (SFS_IS_DIR(inode)) {               
        // 目录需要把 目录下的 文件名 写入数据块           
        dentry_cursor = inode->dentrys;
        offset        = SFS_DATA_OFS(dno);
        while (dentry_cursor != NULL)
        {
            memcpy(dentry_d.fname, dentry_cursor->fname, SFS_MAX_FILE_NAME);
            dentry_d.ftype = dentry_cursor->ftype;
            dentry_d.ino = dentry_cursor->ino;
            if (sfs_driver_write(offset, (uint8_t *)&dentry_d, 
                                 sizeof(struct sfs_dentry_d)) != SFS_ERROR_NONE) {
                SFS_DBG("[%s] io error\n", __func__);
                return -SFS_ERROR_IO;                     
            }
            
            if (dentry_cursor->inode != NULL) {
                sfs_sync_inode(dentry_cursor->inode);
            }

            dentry_cursor = dentry_cursor->brother;
            offset += sizeof(struct sfs_dentry_d);
        }
    }
    else if (SFS_IS_REG(inode)) {
        // 普通文件需要把数据写入 磁盘数据块 
        if (sfs_driver_write(SFS_DATA_OFS(dno), inode->data, 
                             SFS_BLKS_SZ(SFS_DATA_PER_FILE)) != SFS_ERROR_NONE) {
            SFS_DBG("[%s] io error\n", __func__);
            return -SFS_ERROR_IO;
        }
    }
    return SFS_ERROR_NONE;
}
/**
 * @brief 删除内存中的一个inode， 暂时不释放
 * Case 1: Reg File
 * 
 *                  Inode
 *                /      \
 *            Dentry -> Dentry (Reg Dentry)
 *                       |
 *                      Inode  (Reg File)
 * 
 *  1) Step 1. Erase Bitmap     
 *  2) Step 2. Free Inode                      (Function of sfs_drop_inode)
 * ------------------------------------------------------------------------
 *  3) *Setp 3. Free Dentry belonging to Inode (Outsider)
 * ========================================================================
 * Case 2: Dir
 *                  Inode
 *                /      \
 *            Dentry -> Dentry (Dir Dentry)
 *                       |
 *                      Inode  (Dir)
 *                    /     \
 *                Dentry -> Dentry
 * 
 *   Recursive
 * @param inode 
 * @return int 
 */
int sfs_drop_inode(struct sfs_inode * inode) {
    struct sfs_dentry*  dentry_cursor;
    struct sfs_dentry*  dentry_to_free;
    struct sfs_inode*   inode_cursor;

    int byte_cursor = 0; 
    int bit_cursor  = 0; 
    int ino_cursor  = 0;
    boolean is_find = FALSE;

    if (inode == sfs_super.root_dentry->inode) {
        return SFS_ERROR_INVAL;
    }

    if (SFS_IS_DIR(inode)) {
        dentry_cursor = inode->dentrys;
                                                      /* 递归向下drop */
        while (dentry_cursor)
        {   
            inode_cursor = dentry_cursor->inode;
            sfs_drop_inode(inode_cursor);
            sfs_drop_dentry(inode, dentry_cursor);
            dentry_to_free = dentry_cursor;
            dentry_cursor = dentry_cursor->brother;
            free(dentry_to_free);
        }
    }
    else if (SFS_IS_REG(inode) || SFS_IS_SYM_LINK(inode)) {
        for (byte_cursor = 0; byte_cursor < SFS_BLKS_SZ(sfs_super.map_inode_blks); 
            byte_cursor++)                            /* 调整inodemap */
        {
            for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
                if (ino_cursor == inode->ino) {
                     sfs_super.map_inode[byte_cursor] &= (uint8_t)(~(0x1 << bit_cursor));
                     is_find = TRUE;
                     break;
                }
                ino_cursor++;
            }
            if (is_find == TRUE) {
                break;
            }
        }
        if (inode->data)
            free(inode->data);
        free(inode);
    }
    return SFS_ERROR_NONE;
}
/**
 * @brief 
 * 
 * @param dentry dentry指向ino，读取该inode
 * @param ino inode唯一编号
 * @return struct sfs_inode* 
 */
struct sfs_inode* sfs_read_inode(struct sfs_dentry * dentry, int ino) {
    printf("~ hello ~\n");
    struct sfs_inode* inode = (struct sfs_inode*)malloc(sizeof(struct sfs_inode));
    struct sfs_inode_d inode_d;
    struct sfs_dentry* sub_dentry;
    struct sfs_dentry_d dentry_d;
    int    dir_cnt = 0, i;
    if (sfs_driver_read(SFS_INO_OFS(ino), (uint8_t *)&inode_d, 
                        sizeof(struct sfs_inode_d)) != SFS_ERROR_NONE) {
        SFS_DBG("[%s] io error\n", __func__);
        return NULL;                    
    }
    inode->dir_cnt = 0;
    inode->ino = inode_d.ino;
    inode->size = inode_d.size;
    memcpy(inode->target_path, inode_d.target_path, SFS_MAX_FILE_NAME);
    inode->dentry = dentry;
    inode->dentrys = NULL;
    if (SFS_IS_DIR(inode)) {
        dir_cnt = inode_d.dir_cnt;
        for (i = 0; i < dir_cnt; i++)
        {
            if (sfs_driver_read(SFS_DATA_OFS(ino) + i * sizeof(struct sfs_dentry_d), 
                                (uint8_t *)&dentry_d, 
                                sizeof(struct sfs_dentry_d)) != SFS_ERROR_NONE) {
                SFS_DBG("[%s] io error\n", __func__);
                return NULL;                    
            }
            sub_dentry = new_dentry(dentry_d.fname, dentry_d.ftype);
            sub_dentry->parent = inode->dentry;
            sub_dentry->ino    = dentry_d.ino; 
            sfs_alloc_dentry(inode, sub_dentry);
        }
    }
    else if (SFS_IS_REG(inode)) {
        inode->data = (uint8_t *)malloc(SFS_BLKS_SZ(SFS_DATA_PER_FILE));
        if (sfs_driver_read(SFS_DATA_OFS(ino), (uint8_t *)inode->data, 
                            SFS_BLKS_SZ(SFS_DATA_PER_FILE)) != SFS_ERROR_NONE) {
            SFS_DBG("[%s] io error\n", __func__);
            return NULL;                    
        }
    }
    return inode;
}
/**
 * @brief 
 * 
 * @param inode 
 * @param dir [0...]
 * @return struct sfs_dentry* 
 */
struct sfs_dentry* sfs_get_dentry(struct sfs_inode * inode, int dir) {
    struct sfs_dentry* dentry_cursor = inode->dentrys;
    int    cnt = 0;
    while (dentry_cursor)
    {
        if (dir == cnt) {
            return dentry_cursor;
        }
        cnt++;
        dentry_cursor = dentry_cursor->brother;
    }
    return NULL;
}
/**
 * @brief 
 * path: /qwe/ad  total_lvl = 2,
 *      1) find /'s inode       lvl = 1
 *      2) find qwe's dentry 
 *      3) find qwe's inode     lvl = 2
 *      4) find ad's dentry
 *
 * path: /qwe     total_lvl = 1,
 *      1) find /'s inode       lvl = 1
 *      2) find qwe's dentry
 * 
 * @param path 
 * @return struct sfs_inode* 
 */
struct sfs_dentry* sfs_lookup(const char * path, boolean* is_find, boolean* is_root) {
    struct sfs_dentry* dentry_cursor = sfs_super.root_dentry;
    struct sfs_dentry* dentry_ret = NULL;
    struct sfs_inode*  inode; 
    int   total_lvl = sfs_calc_lvl(path);
    int   lvl = 0;
    boolean is_hit;
    char* fname = NULL;
    char* path_cpy = (char*)malloc(sizeof(path));
    *is_root = FALSE;
    strcpy(path_cpy, path);

    if (total_lvl == 0) {                           /* 根目录 */
        *is_find = TRUE;
        *is_root = TRUE;
        dentry_ret = sfs_super.root_dentry;
    }
    fname = strtok(path_cpy, "/");       
    while (fname)
    {   
        lvl++;
        if (dentry_cursor->inode == NULL) {           /* Cache机制 */
            sfs_read_inode(dentry_cursor, dentry_cursor->ino);
        }

        inode = dentry_cursor->inode;

        if (SFS_IS_REG(inode) && lvl < total_lvl) {
            SFS_DBG("[%s] not a dir\n", __func__);
            dentry_ret = inode->dentry;
            break;
        }
        if (SFS_IS_DIR(inode)) {
            dentry_cursor = inode->dentrys;
            is_hit        = FALSE;

            while (dentry_cursor)
            {
                if (memcmp(dentry_cursor->fname, fname, strlen(fname)) == 0) {
                    is_hit = TRUE;
                    break;
                }
                dentry_cursor = dentry_cursor->brother;
            }
            
            if (!is_hit) {
                *is_find = FALSE;
                SFS_DBG("[%s] not found %s\n", __func__, fname);
                dentry_ret = inode->dentry;
                break;
            }

            if (is_hit && lvl == total_lvl) {
                *is_find = TRUE;
                dentry_ret = dentry_cursor;
                break;
            }
        }
        fname = strtok(NULL, "/"); 
    }

    if (dentry_ret->inode == NULL) {
        dentry_ret->inode = sfs_read_inode(dentry_ret, dentry_ret->ino);
    }
    
    return dentry_ret;
}
/**
 * @brief 挂载sfs, Layout 如下
 * 
 * Layout
 * | Super(1) | Inode Map(1) | DATA Map(1) | INODE(1) | DATA(*) |
 * 
 * BLK_SZ = 2 * IO_SZ
 * 
 * 每个Inode占用一个Blk
 * @param options 
 * @return int 
 */
int sfs_mount(struct custom_options options){
    int                 ret = SFS_ERROR_NONE;
    int                 driver_fd;
    struct sfs_super_d  sfs_super_d; 
    struct sfs_dentry*  root_dentry;
    struct sfs_inode*   root_inode;

    int                 inode_num;
    int                 map_inode_blks;
    int                 map_data_blks;
        
    int                 super_blks;
    boolean             is_init = FALSE;

    sfs_super.is_mounted = FALSE;

    // driver_fd = open(options.device, O_RDWR);
    driver_fd = ddriver_open(options.device);

    if (driver_fd < 0) {
        return driver_fd;
    }

    sfs_super.driver_fd = driver_fd;
    ddriver_ioctl(SFS_DRIVER(), IOC_REQ_DEVICE_SIZE,  &sfs_super.sz_disk);
    ddriver_ioctl(SFS_DRIVER(), IOC_REQ_DEVICE_IO_SZ, &sfs_super.sz_io);
    
    root_dentry = new_dentry("/", SFS_DIR);

    if (sfs_driver_read(SFS_SUPER_OFS, (uint8_t *)(&sfs_super_d), 
                        sizeof(struct sfs_super_d)) != SFS_ERROR_NONE) {
        return -SFS_ERROR_IO;
    }   
                                                      /* 读取super */
    if (sfs_super_d.magic_num != SFS_MAGIC_NUM) {     /* 幻数无 */
                                                      /* 估算各部分大小 */

        /**
         * 一个设计的简单例子

            磁盘容量为4MB，逻辑块大小为1024B，那么逻辑块数应该是4MB / 1024B = 4096。
            我们采用直接索引，假设每个文件最多直接索引6个逻辑块来填写文件数据，也就是
            每个文件数据上限是6 * 1024B = 6KB。假设一个文件的索引节点，采用一个逻辑
            块存储（见下面说明）。那么维护一个文件所需要的存储容量是6KB + 1KB = 7KB。
            那么4MB磁盘，最多可以存放的文件数是4MB / 7KB = 585。

            超级块，1个逻辑块。超级块区需要保存刷回磁盘的struct super_block_d，一般
            这个结构体的大小会小于1024B，我们用一个逻辑块作为超级块存储 struct super_block_d即可。

            索引节点位图，1个逻辑块。上述文件系统最多支持585个文件维护，一个逻辑块
            （1024B）的位图可以管理1024 * 8 = 8192个索引节点，完全足够。

            数据块位图，1个逻辑块。上述文件系统总共逻辑块数才4096，一个逻辑块（1024B）
            的位图可以管理8192个逻辑块，足够。

            索引节点区，585个逻辑块。上述文件系统假设一个逻辑块放一个索引节点struct inode_d，
            585个文件需要有585索引节点，也就是585个逻辑块。

            数据块区，3508个逻辑块。剩下的都作为数据块，还剩4096 - 1 - 1 - 1 - 585 = 3508个逻辑块。
            注：struct inode_d的大小一般是比1024B小很多的，一个逻辑块放一个struct inode_d会显得有
            点奢侈，这里是简单起见，同学们可以确定struct inode_d的大小后，自行决定一个逻辑块放多少个
            索引节点struct inode_d。
         * 
         */

        // 超级块，占1个块
        super_blks = 1;
        // Inode Map 占1个块
        map_inode_blks = 1;
        // Data Map 占1个块
        map_data_blks = 1;
        
        /* 布局layout */
        // 总的 inode 数量
        sfs_super_d.max_ino = SFS_BLKS_SZ(map_inode_blks)*8;
        // Inode Map
        sfs_super_d.map_inode_blks  = map_inode_blks;
        sfs_super_d.map_inode_offset = SFS_SUPER_OFS + SFS_BLKS_SZ(super_blks);
        // Data Map
        sfs_super_d.map_data_blks  = map_data_blks;
        sfs_super_d.map_data_offset = sfs_super_d.map_inode_offset + SFS_BLKS_SZ(map_inode_blks);
        // Inode Offset
        sfs_super_d.inode_offset = sfs_super_d.map_data_offset +  SFS_BLKS_SZ(map_data_blks);
        // Data Offset
        sfs_super_d.data_offset = sfs_super_d.inode_offset + SFS_BLKS_SZ(sfs_super_d.max_ino);
        
        sfs_super_d.sz_usage    = 0;        

        // 标记这个磁盘是第一次使用
        is_init = TRUE;
    }    

    sfs_super.max_ino = sfs_super_d.max_ino;
    sfs_super.sz_usage   = sfs_super_d.sz_usage;      /* 建立 in-memory 结构 */
            
    // 读取 Inode Map
    sfs_super.map_inode_blks = sfs_super_d.map_inode_blks;
    sfs_super.map_inode_offset = sfs_super_d.map_inode_offset;
    sfs_super.map_inode = (uint8_t *)malloc(SFS_BLKS_SZ(sfs_super.map_inode_blks));
    if (TRUE == is_init){  // 磁盘第一次使用，我们把 Inode Map 清零
        SFS_DBG("map_inode Init 0\n");
        memset(sfs_super.map_inode, 0, SFS_BLKS_SZ(sfs_super.map_inode_blks));
    }else{ // 不是第一次使用，我们从磁盘读取之前保存的数据
        SFS_DBG("map_inode Read From Disk\n");
        if (sfs_driver_read(sfs_super.map_inode_offset, (uint8_t *)(sfs_super.map_inode), 
                                SFS_BLKS_SZ(sfs_super.map_inode_blks)) != SFS_ERROR_NONE) {
                return -SFS_ERROR_IO;
            }
    }
    
    // 读取 Data Map
    sfs_super.map_data_blks = sfs_super_d.map_data_blks;
    sfs_super.map_data_offset = sfs_super_d.map_data_offset;
    sfs_super.map_data = (uint8_t *)malloc(SFS_BLKS_SZ(sfs_super.map_data_blks));
    if (TRUE == is_init){  // 磁盘第一次使用，我们把 Data Map 清零
        SFS_DBG("map_data Init 0\n");
        memset(sfs_super.map_data, 0, SFS_BLKS_SZ(sfs_super.map_data_blks));
    }else{ // 不是第一次使用，我们从磁盘读取之前保存的数据
        SFS_DBG("map_data Read From Disk\n");
        if (sfs_driver_read(sfs_super.map_data_offset, (uint8_t *)(sfs_super.map_data), 
                                SFS_BLKS_SZ(sfs_super.map_data_blks)) != SFS_ERROR_NONE) {
                return -SFS_ERROR_IO;
            }
    }
    
    sfs_super.inode_offset = sfs_super_d.inode_offset;
    sfs_super.data_offset = sfs_super_d.data_offset;
       
    if (is_init) {                                    /* 分配根节点 */
        root_inode = sfs_alloc_inode(root_dentry);
        sfs_sync_inode(root_inode);
    }
       
    root_inode            = sfs_read_inode(root_dentry, SFS_ROOT_INO);
    root_dentry->inode    = root_inode;
    sfs_super.root_dentry = root_dentry;
    sfs_super.is_mounted  = TRUE;   


    SFS_DBG("Max Inode   Number: %d\n", sfs_super.max_ino);
    SFS_DBG("Super Block Offset: %d\n", SFS_SUPER_OFS);
    SFS_DBG("Inode Map   Offset: %d\n", sfs_super.map_inode_offset);
    SFS_DBG("Data  Map   Offset: %d\n", sfs_super.map_data_offset);
    SFS_DBG("Inode       Offset: %d\n", sfs_super.inode_offset);
    SFS_DBG("Data        Offset: %d\n", sfs_super.data_offset);  

    // sfs_dump_map();
    return ret;
}
/**
 * @brief 
 * 
 * @return int 
 */
int sfs_umount() {
    struct sfs_super_d  sfs_super_d; 

    if (!sfs_super.is_mounted) {
        return SFS_ERROR_NONE;
    }

    // 写入目录和文件数据
    SFS_DBG("\n");
    SFS_DBG("umount sync inode\n");
    sfs_sync_inode(sfs_super.root_dentry->inode);     /* 从根节点向下刷写节点 */
                                                    
    // 写入 Super Block                                                     
    sfs_super_d.magic_num          = SFS_MAGIC_NUM;
    sfs_super_d.sz_usage           = sfs_super.sz_usage;
    sfs_super_d.max_ino            = sfs_super.max_ino;
    sfs_super_d.map_inode_blks     = sfs_super.map_inode_blks;
    sfs_super_d.map_inode_offset   = sfs_super.map_inode_offset;
    sfs_super_d.map_data_blks      = sfs_super.map_data_blks;
    sfs_super_d.map_data_offset    = sfs_super.map_data_offset;
    sfs_super_d.inode_offset       = sfs_super.inode_offset;
    sfs_super_d.data_offset        = sfs_super.data_offset;
    
    SFS_DBG("umount write super block\n");
    if (sfs_driver_write(SFS_SUPER_OFS, (uint8_t *)&sfs_super_d, 
                     sizeof(struct sfs_super_d)) != SFS_ERROR_NONE) {
        return -SFS_ERROR_IO;
    }

    // 写入 Inode Map
    SFS_DBG("umount write inode map\n");
    if (sfs_driver_write(sfs_super_d.map_inode_offset, (uint8_t *)(sfs_super.map_inode), 
                         SFS_BLKS_SZ(sfs_super_d.map_inode_blks)) != SFS_ERROR_NONE) {
        return -SFS_ERROR_IO;
    }
    free(sfs_super.map_inode);

    // 写入 Data Map 
    SFS_DBG("umount write data map\n");
    if (sfs_driver_write(sfs_super_d.map_data_offset, (uint8_t *)(sfs_super.map_data), 
                         SFS_BLKS_SZ(sfs_super_d.map_data_blks)) != SFS_ERROR_NONE) {
        return -SFS_ERROR_IO;
    }    
    free(sfs_super.map_data);

    ddriver_close(SFS_DRIVER());

    return SFS_ERROR_NONE;
}
