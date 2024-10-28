#ifndef _TYPES_H_
#define _TYPES_H_

/******************************************************************************
* SECTION: Type def
*******************************************************************************/
typedef int          boolean;
typedef uint16_t     flag16;

typedef enum sfs_file_type {
    SFS_REG_FILE,
    SFS_DIR,
    SFS_SYM_LINK
} SFS_FILE_TYPE;
/******************************************************************************
* SECTION: Macro
*******************************************************************************/
#define TRUE                    1
#define FALSE                   0
#define UINT32_BITS             32
#define UINT8_BITS              8

#define SFS_MAGIC_NUM           0x52415453  
#define SFS_SUPER_OFS           0
#define SFS_ROOT_INO            0



#define SFS_ERROR_NONE          0
#define SFS_ERROR_ACCESS        EACCES
#define SFS_ERROR_SEEK          ESPIPE     
#define SFS_ERROR_ISDIR         EISDIR
#define SFS_ERROR_NOSPACE       ENOSPC
#define SFS_ERROR_EXISTS        EEXIST
#define SFS_ERROR_NOTFOUND      ENOENT
#define SFS_ERROR_UNSUPPORTED   ENXIO
#define SFS_ERROR_IO            EIO     /* Error Input/Output */
#define SFS_ERROR_INVAL         EINVAL  /* Invalid Args */

#define SFS_MAX_FILE_NAME       128
#define SFS_INODE_PER_FILE      1
#define SFS_DATA_PER_FILE       1       //一个文件只占用一个数据块 1024，目的是为了简化设计
#define SFS_DEFAULT_PERM        0777

#define SFS_IOC_MAGIC           'S'
#define SFS_IOC_SEEK            _IO(SFS_IOC_MAGIC, 0)

#define SFS_FLAG_BUF_DIRTY      0x1
#define SFS_FLAG_BUF_OCCUPY     0x2
/******************************************************************************
* SECTION: Macro Function
*******************************************************************************/
#define SFS_IO_SZ()                     (sfs_super.sz_io)
#define SFS_BLK_SZ()                    (SFS_IO_SZ() * 2)
#define SFS_DISK_SZ()                   (sfs_super.sz_disk)
#define SFS_DRIVER()                    (sfs_super.driver_fd)

#define SFS_ROUND_DOWN(value, round)    ((value) % (round) == 0 ? (value) : ((value) / (round)) * (round))
#define SFS_ROUND_UP(value, round)      ((value) % (round) == 0 ? (value) : ((value) / (round) + 1) * (round))

#define SFS_BLKS_SZ(blks)               ((blks) * SFS_BLK_SZ())
#define SFS_ASSIGN_FNAME(psfs_dentry, _fname)\ 
                                        memcpy(psfs_dentry->fname, _fname, strlen(_fname))

// 获取 inode 保存的磁盘 offset
#define SFS_INO_OFS(ino)                (sfs_super.inode_offset + (ino) * SFS_BLK_SZ())
// 获取 data 保存的磁盘 offset 
#define SFS_DATA_OFS(dno)               (sfs_super.data_offset +  (dno) * SFS_BLK_SZ())

#define SFS_IS_DIR(pinode)              (pinode->dentry->ftype == SFS_DIR)
#define SFS_IS_REG(pinode)              (pinode->dentry->ftype == SFS_REG_FILE)
#define SFS_IS_SYM_LINK(pinode)         (pinode->dentry->ftype == SFS_SYM_LINK)
/******************************************************************************
* SECTION: FS Specific Structure - In memory structure
*******************************************************************************/
struct sfs_dentry;
struct sfs_inode;
struct sfs_super;

struct custom_options {
	const char*        device;
	boolean            show_help;
};

struct sfs_inode
{
    int                ino;                           /* 在 Inode Map 中的下标 */
    int                dno;                           /* 在 Data Map 中的下标 */
    int                size;                          /* 文件已占用空间 */
    char               target_path[SFS_MAX_FILE_NAME];/* store traget path when it is a symlink */
    int                dir_cnt;
    struct sfs_dentry* dentry;                        /* 指向该inode的dentry */
    struct sfs_dentry* dentrys;                       /* 所有目录项 */
    uint8_t*           data;           
};  

struct sfs_dentry
{
    char               fname[SFS_MAX_FILE_NAME];
    struct sfs_dentry* parent;                        /* 父亲Inode的dentry */
    struct sfs_dentry* brother;                       /* 兄弟 */
    int                ino;
    struct sfs_inode*  inode;                         /* 指向inode */
    SFS_FILE_TYPE      ftype;
};

/** 这个是内存结构，方便平时操作，下面  struct sfs_super_d  是对应磁盘结构 
 * 
 * | Super(1) | Inode Map(1) | DATA Map(1) | INODE(1) | DATA(*) |
 * 
 *  在 sfs_mount 的时候从磁盘读取，然后加载到这里
 *  在 sfs_umount 的时候重新写回到磁盘
 * 
*/
struct sfs_super
{
    int                driver_fd;
    
    int                sz_io;
    int                sz_disk;
    int                sz_usage;
    
    int                max_ino;

    // Inode Map
    uint8_t*           map_inode;
    int                map_inode_blks;
    int                map_inode_offset;
    
    // Data Map
    uint8_t*           map_data;
    int                map_data_blks;
    int                map_data_offset;
    
    // Inode 
    int                inode_offset;

    // Data 
    int                data_offset;

    boolean            is_mounted;

    struct sfs_dentry* root_dentry;
};

static inline struct sfs_dentry* new_dentry(char * fname, SFS_FILE_TYPE ftype) {
    struct sfs_dentry * dentry = (struct sfs_dentry *)malloc(sizeof(struct sfs_dentry));
    memset(dentry, 0, sizeof(struct sfs_dentry));
    SFS_ASSIGN_FNAME(dentry, fname);
    dentry->ftype   = ftype;
    dentry->ino     = -1;
    dentry->inode   = NULL;
    dentry->parent  = NULL;
    dentry->brother = NULL;                                            
}
/******************************************************************************
*  这个是磁盘结构，和保存在磁盘上的数据一一对应
*  在 sfs_mount 的时候从磁盘读取，然后加载到这里
*  在 sfs_umount 的时候重新写回到磁盘
*
* SECTION: FS Specific Structure - Disk structure
* | Super(1) | Inode Map(1) | DATA Map(1) | INODE(1) | DATA(*) |
*******************************************************************************/
struct sfs_super_d
{
    uint32_t           magic_num;
    uint32_t           sz_usage;
    
    uint32_t           max_ino;
    // Inode Map(1) 
    uint32_t           map_inode_blks;
    uint32_t           map_inode_offset;
    // DATA Map(1) 
    uint32_t           map_data_blks;
    uint32_t           map_data_offset;    
    // INODE
    uint32_t           inode_offset;
    // DATA
    uint32_t           data_offset;
};

struct sfs_inode_d
{
    uint32_t           ino;                           /* 在 Inode Map 中的下标 */
    uint32_t           dno;                           /* 在 Data Map 中的下标 */
    uint32_t           size;                          /* 文件已占用空间 */
    char               target_path[SFS_MAX_FILE_NAME];/* store traget path when it is a symlink */
    uint32_t           dir_cnt;
    SFS_FILE_TYPE      ftype;   
};  

struct sfs_dentry_d
{
    char               fname[SFS_MAX_FILE_NAME];
    SFS_FILE_TYPE      ftype;
    uint32_t           ino;                           /* 指向的ino号 */
};  


#endif /* _TYPES_H_ */