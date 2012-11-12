afpfs-ng
========

afpfs-ng
使用afpfs-ng

    如何使用afpfs-ng？
    首先先运行SERVER DAEMON

    afpfsd

    然后运行mount命令加载afp

    afp_client mount -u alpha -p 111111 172.18.83.222:shared $mount_point 2>&1 >/dev/null
    or
    mount_afp "afp://alpha:111111@172.18.83.222/shared" $mount_point 2>&1 > /dev/null 

    umount文件系统

    afp_client unmount $mount_point
    or
    umount -l $mount_point

我们如何找到局域网内的AFP SERVER

    利用mDNSClientPosix命令找到,存在的server

    执行
    /opt/bin/mDNSClientPosix &
    会在/tmp/下面生成文件
    /tmp/afp_server_list.log
    内容为

    state='1', name='MacBook Air', ip='172.18.83.222'

    这就知道在局域网内SERVER的IP和名字。

    利用afp_client get_server 找到server上的volumes

    执行
    afp_client get_volumes "afp://alpha:111111@172.18.83.222" &
    之后会在/tmp/下面生成文件
    /tmp/get_afp_volume_list
    内容为

    Macintosh HD
    shared
    alpha

    这就知道在局域网内某IP，SERVER的VOLUMES有哪些

