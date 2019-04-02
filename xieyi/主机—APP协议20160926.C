主机与App通讯命令

NodeType  
 	0x00 - switch
	0x01 - socket
	0x02 - curtain
	0x03 - lights

Node mac  节点mac地址（2） 
设备控制命令：下传数据
  
||**********************************************************************************||
1.配置手动场景：(max64)
  |OPCODE(1)||+LEN||+Manual Scene ID(1)||+Slave device Type(1||)Slave node mac(2)+||slave device id(1)||+Slave NodeValue(1)||...
  +||+Slave device Type(1||)Slave node mac(2)+||slave device id(1)||+Slave NodeValue(1)
  
  OPCODE
    协议头：0x01  
  LEN：
    消息长度    
  Manual Scene ID
   手动场景序列号：00  表示新注册的ID  
  Slave node mac
    节点mac地址
  Slave device Type     slave device id                              Slave NodeValue
    继电器 0x02           5~8    // 节点设备状态                        0关，1开（1）
                                    低4位本地设备状态值，
                                    第5位夜灯状态值    
    小夜灯 0x06           0                                             0关，1开  （1）
    背光灯 0x09           0/1 //调光  0下调，1上调                       亮度值   （1）
    灯具   0x10           0/1 //调光  0下调，1上调                       亮度值   （1）
                          2   //开/关+状态值（0关，1开)(1)，             0关，1开 （1）        
                          3   //调色+RGB(3)                              RGB值    （3）
    红外码: 
        电视   0x11          一串红外码引导索引号           
        空调   0x12          一串红外码引导索引号
        机顶盒 0x13          一串红外码引导索引号 
        
||***********************************************************************************||
应答手动配置包       
||opcode(1)||+LEN(1)||+Manual Scene ID(1)||  
 
 opcode
   协议头：0x02
 LEN
   消息包长度
 Manual Scene ID
   生成手动场景号      
        
2.手动场景操作：
 ||opcode(1)||+LEN(1)||+Manual Scene ID(1)||
 OPCODE
    协议头：0x03    
 LEN:
 	  消息包长度  
 Manual Scene ID
   手动场景序列号
   
||******************************************************************************||  
删除手动场景：
||opcode(1)||+LEN||+Manual Scene ID||
 
opcode
  协议头：0x04
LEN:
	 消息包长度  
Manual Scene ID
  手动场景序列号   
  
||******************************************************************************||
删除手动场景应答
||opcode(1)||+LEN||+Manual Scene ID||+value||
opcode
  协议头：0x05
LEN:
	 消息包长度  
Manual Scene ID
  手动场景序列号   
value
  0x01   删除手动场景成功
  0x02   删除手动场景失败
  
||******************************************************************************||  
      
3.配置自动场景：
||OPCODE（1）+||LEN+||auto Scene ID+||Trigger mode+||触发条件+||限制条件+||执行任务|| 
 
OPCODE
 协议头：0x06
LEN：
消息包长度 
auto Scene ID
 自动场景序列号：00 新注册ID 
     
Trigger mode        0 满足任意一个条件
                    1 满足所有条件
 
触发条件：                    
Trigger Type     Trigger condition   
  0xE0            +|node MAC(2)|+人体感应(0x15)|+状态|+Time(继续时间长度)（２）|
                                                  0 非   持续时间(15s~2h)
                                                  1 是 
  0xE1            +|---Day(1)---|---hour(1)---|---miniter(1)|
                    Day(1)    :低7位依次表示 日，一，二，三，四，五，六 
                    hour(1)   :0~23
                    miniter(1):0~59
  0XE2            +|node MAC(2)|+光感(0x11)|+|范围类型|+    光感强度(1)| 
                                              大于0x01     光感强度（1）
                                              小于0x02     光感强度（1）
                                              范围内0x03   光感强度（2）前面放小于后面值
                                              范围外0x04   光感强度（2）前面放小于后面值
                                              
  0xE3            +|node MAC(2)|+温度(0x16)|+|范围类型|+   温度(1)|   
                                              大于0x01     温度（1）
                                              小于0x02     温度（1）
                                              范围内0x03   温度（2）前面放小于后面值
                                              范围外0x04   温度（2）前面放小于后面值
                                              
  0xE4            +|node MAC(2)|+ device ID+     状态（1）+时间（持续时间）（2）
                                  继电器(5~8)    0 关        时间（分）                             
                                                 1 开        时间（分）   
                                               
  0xE5            +Mobile phone Type 
                        0 （离家）
                        1  归家  
                        2  翻动
                        3  短信
                        4  来电
                        5  摇动
                        
限制条件：
|---Day(1)---|---hour0(1)---|---miniter0(1)|---|---hour1(1)---|---miniter1(1)|+。。。

Day(1)    :低7位依次表示 日，一，二，三，四，五，六 
hour(1)   :0~23
miniter(1):0~59   


执行任务：
+++|---Slave device Type(1)---|---Slave node mac(2)---|---slave device id---|--Slave NodeValue|。。。。 

Slave node mac
  从节点mac地址
Slave device Type    slave device id                                     Slave NodeValue
  继电器 0x02           5~8  // 节点设备状态 低4位本地设备状态值           0 关，1开
  小夜灯 0x06           0                                                  0 关，1开
  背光灯 0x09           0/1    //调光  0下调，1上调                        亮度值(1)
  灯具   0x10           0/1    //调光  0下调，1上调                        亮度值(1)
                        2      //开/关+状态值（0关，1开)(1)，              0关，1开         
                        3      //调色+RGB(3)                               RGB值(3) 
                        
红外码: 
     电视           0x11          一串红外码引导索引号           
     空调           0x12          一串红外码引导索引号
     电视机顶盒     0x13          一串红外码引导索引号 
     DVD播放机      0x14          一串红外码引导索引号 
     投影仪         0x15          一串红外码引导索引号 
     互联网机顶盒   0x16          一串红外码引导索引号 
     风扇           0x17          一串红外码引导索引号 
     音响           0x18          一串红外码引导索引号 
     扫地机         0x19          一串红外码引导索引号 
     空气净化器     0x1a          一串红外码引导索引号 
     照相机         0x1b          一串红外码引导索引号 
 
||****************************************************************************||
主机应答自动场景配置ID 
 |OPCODE（1）+||LEN+||auto Scene ID||
 OPCODE
  协议头：0x07
 LEN 
  消息包长度
 auto Scene ID 
     
||****************************************************************************||    
自动场景使能操作：
 |OPCODE（1）+||LEN+||auto Scene ID+||SceneEnable||
 OPCODE
   协议头：0x08
 LEN：
   消息长度  
 auto Scene ID
  自动场景序列号
 SceneEnable
    0 取消次自动场景
    1 使能自动场景      
     
||******************************************************************************||
删除自动场景     
 |OPCODE（1）+||LEN+||auto Scene ID+||
 OPCODE
   协议头：0x09
 LEN  
 消息长度   
 auto Scene ID
  自动场景序列号
     
||*******************************************************************************||
APP 开关本地 操作 单指令控制
||opcode||+LEN||+Node mac||+Node value||
 opcode 
    协议头：0x0a
 LEN
    消息长度   
 Node mac
    节点mac地址 
 Node value  //低4位 本地四负载；第5位表示小夜灯
   0 关
   1 开 
   
||*******************************************************************************||
设置人体感应使能
opcode ||+LEN||+Node type||+Node mac(2)||+Node Enable
opcode 
协议头：0x0b
LEN
消息包长度
Node mac
  节点mac地址
Node type
  节点类型：0x00 开关  
Node Enable
 1使能，0取消；//第0位 人体感应
 
||******************************************************************************||
震动设置

开关震动检测关联小夜灯和蜂鸣器（只能关联自己的）
 opcode ||+LEN||+Node type||+Node mac(2)||+Node Enable|| 
 opcode 
协议头：0x0c
LEN
消息包长度
Node mac
  节点mac地址
Node type
  节点类型：0x00 开关  
Node Enable （第0位 小夜灯，第1位 蜂鸣器）
 1使能，
 0取消；
 
||*******************************************************************************||
设置开关蜂鸣器使能
opcode||+LEN||+Node type||+Node mac(2)||+Node Buzz Enable||
 
协议头：0x0d
LEN
消息包长度
Node mac
  节点mac地址
Node type
  节点类型：0x00 开关  
Node Buzz Enable （第0位蜂鸣器）
 1使能，
 0取消； 
||*******************************************************************************||
开关背光灯使能设置
opcode||+LEN||+Node type||+Node mac（2）||+Node LED Enable 
 
协议头：0x0e
LEN
消息包长度
Node mac
  节点mac地址
Node type
  节点类型：0x00 开关  
Node Buzz Enable （第0位背光灯）
 1使能，
 0取消；
||******************************************************************************||
开关背光灯联动本地人体感应使能设置
 opcode||+LEN||+Node type||+Node mac(2)||+Node LED Enable 
 
协议头：0x0f
LEN
消息包长度
Node mac
  节点mac地址
Node type
  节点类型：0x00 开关  
Node Buzz Enable （第0位背光灯）
 1使能，
 0取消；

||*******************************************************************************||
开关背光灯联动本地光感使能设置
 opcode||+LEN||+Node type||+Node mac(2)||+Node LED Enable 
 
协议头：0x10
LEN
消息包长度
Node mac
  节点mac地址
Node type
  节点类型：0x00 开关  
Node Buzz Enable （第0位背光灯）
 1使能，
 0取消；

||*******************************************************************************||    
 开关本地背光灯调光  
opcode||+LEN||+Node type||+Node mac(2)||+Light intensity||
opcode
 协议头：0x11
LEN
消息包长度 
 Node mac
 开关节点mac
 Node type
  节点类型：0x00 开关  
 Light intensity
 光亮度值  
 
||*******************************************************************************||
开关手势使能设置
||opcode||+LEN||+Node type(1)||+Node mac(2)||+Node Enable||  
 opcode 
 协议头：0x12
 LEN:
 	消息包长度
 Node mac
    节点mac地址 
 Node type
    节点类型：0x00  开关      
 Node value  // 手势
   0 取消
   1 使能     
   
||*******************************************************************************||
开关节点手势关联设置
手势只能关联灯具或开关本地负载，二者节点类型选其一 
||opcode(1)||+LEN(1)||+Mester Node Type(1)||+Mester Node mac(2)||+Mester Gesture type(1)||+ Slave Node Type(1) ||+Slave Node mac(2)||+Node device Type(1)||

opcode 
 协议头：0x13
LEN
　消息包长度 
Mester Node mac
  开关节点mac
Mester Node Type
 0x00  (开关类型)
Node Gesture type　
低四位依次表示上　下　左　右　   第4位 人体感应， 第5位光检测，第6位地震 ０未设置，１已设置   

Slave Node mac
  从节点mac地址
Slave Node Type 
/*       --wangssmm  20160910
灯具 0x03 						Node device Type(1)　  
												顺序变换颜色　0x01
												逆序变换颜色  0x02
												亮度增强      0x03
												亮度减弱      0x04
												冷色          0x05
												暖色          0x06
												开            0x07
												关            0x08
												开／关切换    0x09
												无            0x00
*/					
//++wangssmm 20160910							
灯具   0x03             0/1 调光 0下调，1上调          亮度值(0~255)
                           2  开/关                      状态值  0关，1开
                           3  调光                       亮度值(0~255)
                           4  调色                       RGB(3) 												
开关0x00（本身）	 	    Node device Type(1)            Node device value                                                                   					
                             第一路负载 ox00                0关 1开 
                             第二路负载 0x01                0关 1开
                             第三路负载 0x02                0关 1开
                             第四路负载 0x04                0关 1开
                                小夜灯  0x05
                                无
||******************************************************************************|| 
高级关联配置

本地按键关联 一个按键最多关联1项
||opcode(1)||+LEN||+Mester Node mac(2)||+Mester device ID(1)||+Slave Node mac(2)||+slave device ID(1) ||+Relate Mode(1)||
opcode 
 协议头 0x14
LEN 
 消息长度 
Mester Node mac
 开关节点mac地址
Mester device ID
  本地触摸ID号（5~8）
Slave Node mac   
 关联节点mac地址
slave device ID 
 关联本地触摸ID号0（5~8） 
 Relate Mode      关联方式
   0x00              无
   0x01           双向同步关联
   0x02           单向同步关联
   0x03           双向反向关联
   0x04           单向反向关联  
||*********************************************************************|| 
 区域功能关联(最多64)
||opcode||+LEN||+DistrictID||+Slave Node mac0（2）||+slave device ID0 ||。。。+Slave Node mac1（2）||+slave device ID1 ||

opcode
 协议头 0x15
LEN 
消息包长度 
DistrictID:
	区域ID：0x00 新注册区域ID
Slave Node mac0
  关联开关节点mac地址
slave device ID0
  关联开关本地负载（5~8）
  
||**********************************************************************||
主机应答区域ID
||opcode(1)||+LEN||+DistrictID(1)||
 
 opcode 
   协议头：0x16
 LEN 
   消息包长度
 DistrictID：
   区域ID     
  
||**********************************************************************||
区域操作
||opcode(1)||+LEN||+DistrictID(1)||+Node value(1)||
 opcode 
    协议头：0x17
 LEN 
    消息包长度
 DistrictID：
     区域ID   
 Node value 区域操作
    0  关        
    1  开
  
||*******************************************************************************||
节点状态查询
||opcode(1)||+LEN||+Node type(1)||+Node mac(2)||

opcode
 协议头：0x18
LEN 
 消息包长度 
Node mac:
 节点mac地址
Node type
 节点类型： 	0x00 - switch
            	0x01 - socket
	            0x02 - curtain
	            0x03 - lights
||------------------------------------------------------------------------------||

节点状态反馈
||opcode(1)||+LEN||+Node type(1)||+Node mac(2)||+Node value(1)||

opcode
 协议头：0x19
LEN
 消息包长度 
Node mac:
 节点mac地址
Node type
 节点类型： 	0x00 - switch
            	0x01 - socket
	            0x02 - curtain
	            0x03 - lights
Node value
 节点状态：0 关，1 开    

||*******************************************************************************||
开关节点温度查询
||opcode(1)||+LEN||+Node type(1)||+Node mac(2)||

opcode
 协议头：0x1a
 LEN
 消息包长度
Node mac:
 节点mac地址
Node type
 节点类型：0x00 - switch

||--------------------------------------------------------------------------------||
开关温度上传
||opcode(1)||+LEN||+Node type(1)||+Node mac(2)||+temperature value(1)||

opcode
 协议头：0x1b
LEN
消息包长度
Node mac:
 节点mac地址
Node type
 节点类型：0x00 - switch
             	
temperature value：
  温度值(0~255)

||*******************************************************************************||
开关地震信息查询
||opcode(1)||+LEN||+Node type(1)||+Node mac(2)||

opcode
 协议头：0x1c
LEN
消息包长度 
Node mac:
 节点mac地址
Node type
 节点类型:0x00 - switch  

||--------------------------------------------------------------------------------||
开关震动信息上传
||opcode(1)||+LEN||+Node type(1)||+Node mac(2)||+shock value(2)||+ shock Event(1)||

opcode
 协议头：0x1d
LEN
消息包长度
Node mac:
 节点mac地址
Node type
 节点类型:0x00 - switch          	
shock value：
  震动值(0~6535)
shock Event：
  地震事件：0x00 未发生，0x01 发生
  
||*******************************************************************************||
人体感应信息查询
||opcode(1)||+LEN||+Node type(1)||+Node mac(2)||

opcode
 协议头：0x1e
LEN
 消息包长度 
Node mac:
 节点mac地址
Node type
 节点类型:0x00 - switch  
||-------------------------------------------------------------------------------|| 	
人体感应信息上传
||opcode(1)||+LEN||+Node type(1)||+Node mac(2)||+ Human moving Event(1)||

opcode
 协议头：0x1f
LEN
 消息包长度 
Node mac:
 节点mac地址
Node type
 节点类型:0x00 - switch  
Human moving Event：
  人体监测事件：0x00 未发生，0x01 发生

||*******************************************************************************|| 
光感值查询
||opcode(1)||+LEN||+Node type(1)||+Node mac(2)||

opcode
 协议头：0x20
 LEN
 消息包长度
Node mac:
 节点mac地址
Node type
 节点类型:0x00 - switch  
 	
 	
||******************************************************************************||
光感值应答
||opcode(1)||+LEN||+Node type(1)||+Node mac(2)||+value||
opcode
 协议头：0x21
LEN
 消息包长度 
Node mac:
 节点mac地址
Node type
 节点类型:0x00 - switch  
valve：
  光感值（0~130） 	

||------------------------------------------------------------------------------|| 	
 	
设置开关光感阀门值
||opcode(1)||+LEN||+Node type(1)||+Node mac(2)||+ Light valve(1)||

opcode
 协议头：0x22
LEN
 消息包长度 
Node mac:
 节点mac地址
Node type
 节点类型:0x00 - switch  
Light valve：
  光感阀门值（0~130）
  
||-------------------------------------------------------------------------------|| 
开关光感阀值应答 
||opcode(1)||+LEN||+Node type(1)||+Node mac(2)||+ value||  

opcode
 协议头：0x23
LEN
 消息包长度 
Node mac:
 节点mac地址
Node type
 节点类型:0x00 - switch  
Valve：
  0x01 光感阀门值设置成功
  0x02 光感阀门值设置失败
  
||********************************************************************************||
上传光感事件
||opcode(1)||+LEN||+Node type(1)||+Node mac(2)||+ Light Event(1)||
opcode
 协议头：0x24
LEN
 消息包长度 
Node mac:
 节点mac地址
Node type
 节点类型:0x00 - switch  
Light Event：
  光感事件：0x00 未发生，0x01 发生  

||*******************************************************************************||
获取软件/硬件版本
||opcode(1)||+LEN||+Node type(1)||+Node mac(2)||+ Software version(1)||+Hardware version(1)||

opcode
 协议头：0x25
LEN
 消息包长度 
Node mac:
 节点mac地址
Node type
 节点类型：0x00 - switch
           0x01 - socket
	         0x02 - curtain
	         0x03 - lights
Software version：
 软件版本号:0x01	 
Hardware version：
硬件版本号:0x01  
	
	
  
||*******************************************************************************||
场景关联映射（场景学习）
  
||opcode(1)||+LEN||+Mester Node mac(2)||+Mester device ID(1)||+Slave Node mac(2)||+slave device ID(1) || 

opcode
协议头 0x26
LED
 消息包长度
Mester Node ID
  开关节点mac地址
Mester device ID
  区域触摸ID（1~4）
Slave Node ID0
  关联开关节点mac地址
slave device ID0
  关联开关区域触摸ID（1~4）
  
||*******************************************************************************||    
设置手势场景
||opcode||+LEN||+Mester Node mac(2)||+Mester Gesture type(1)||+ Mester Node Type:(1)||+ Slave Node mac(2)||+slave device id(1)||+Slave NodeValue(1)||。。。
   
opcode:
 协议头 0x27
LEN: 
	除包头外数据长度
Mester Node MAC:
 节点mac地址 
Mester Node Type:
 开关节点类型 0x00 
Mester Gesture type:
   0x09    上
   0x0a    下
   0x0b    左
   0x0C    右
Slave Node MAC:
 关联开关节点mac地址 
 
Slave device Type       +slave device id                               +Slave NodeValue
    继电器 0x02           5~8    // 节点设备状态                        0关，1开（1）
                                    低4位本地设备状态值，
                                    第5位夜灯状态值    
    小夜灯 0x06           0                                             0关，1开  （1）
    背光灯 0x09           0/1 //调光  0下调，1上调                       亮度值   （1）
    灯具   0x10           0/1 //调光  0下调，1上调                       亮度值   （1）
                          2   //开/关+状态值（0关，1开)(1)，             0关，1开 （1）        
                          3   //调色+RGB(3)                              RGB值    （3）
    红外码: 
        电视  0x11          一串红外码引导索引号           
        空调  0x12          一串红外码引导索引号
        机顶盒0x13          一串红外码引导索引号 
        
||**********************删除手势场景*************************||        
||opcode(1)|LEN|+Mester Node mac(2)||+Mester Gesture type(1)||+ Mester Node Type(1)
opcode:
 协议头 0x28
Mester Node MAC:
 节点mac地址 
Mester Node Type:
 开关节点类型 0x00 
Mester Gesture type:
   0x09    上
   0x0a    下
   0x0b    左
   0x0C    右        
        
||*****************************************************************************************||
app 灯具控制
||opcode(1)|len|+Node Type(1)||+Node mac(2)||+Node device ID||+Node device value||

opcode: 
协议头：0x29
Node mac: 
 节点mac地址
Node Type:  
 节点类型： 0x03 
+Node device ID                     +Node device value 
  0/1 //调光  0下调，1上调              光亮度0~255（1）
  2   //开/关+状态值                    0关，1开    (1)，                       
  3   //调色                            RGB(3)
  

//****************************************************************************************************************//        
上传数据

开关节点信息
||opcode||+LEN||+Node type||+Node mac(2)||

opcode 
 协议头：0x2A
LEN
消息包长度 
Node mac
 节点物理地址
 
 NodeType                   Data Type(wangsm20160926++)                value
 0x00   开关	                0x01  节点设备状态值 		       +NodeValue(1)        节点设备状态 低4位本地设备状态值，第5位夜灯状态值 ，第七位过零信号
		                          0x02  节点震动值               +震动幅度(2) 
	                          	0x03  节点温度值               +Temperature(1)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
		                          0x04  节点人体感应         		 +humam(1)
		                          0x05  节点背光灯               +亮度(1)		
		
 NodeType  0x01   //插座
     + NodeValue（1）	      节点设备状态 0开，1关    第七位过零信号
 NodeType  0x02   //窗帘
     + NodeValue（1）	      节点设备状态 0开，1关    第七位过零信号
 NodeType  0x03   //灯具
     + NodeValue(1)   0 关，1 开  
     

||**********************删除区域************************************||
||opcode(1)||+LEN||+domain ID(1)||

 opcode:
 协议头：0x2c
 LEN
 消息包长度
domain ID：
 区域ID     
 
||******************************************************************||
操作电视红外控制
||opcode(1)||+LEN||+Node type(1)||+Node mac(2)||+IR ID||

opcode 
 协议头：0x2d
LEN 
 消息包长度
Node mac
 节点地址
Node type
  节点类型：0x00
IR　ID
　0x01     音量+
  0x02     音量-
  0x03　   频道+
  0x04     频道-
  0x05     power 开
  0x06     POwer 关 
||*******************************************************************||
操作空调红外控制
||opcode(1)||+LEN||+Node type(1)||+Node mac(2)||+worke mode||+IR ID||

opcode 
 协议头：0x2e 
LEN 
 消息包长度
Node mac
 节点地址
Node type
  节点类型：0x00
work mode IR_ID:
	0x01   　　　　　　  制冷 
	0x02    　　　　　　 制热
	0x03     　　　　　　自动 
  0x04    　　　　　　 扫风
IR_ID
　0x01     温度+
  0x02     温度-
  0x03　   风速+
  0x04     风速-
  0x05     power 开
  0x06     POwer 关 

||************************************************************||
节点注册信息上传请求
||opcode(1)||+LEN||+Node mac(2)||
opcode
 协议头：0x2f
LEN
 消息包长度：
Node mac(2)
 节点地址  
||************************************************************||
节点信息注册
||opcode(1)||+LEN||+Node mac(2)||+Software version(1)||+Hardware version(1)||+Node type(1)||+NODE_DEVICE_Count||+NodeValue(1)||
opcode 
协议头：0x30
LEN 
消息包长度
Node mac
节点点物理地址
Software version
软件版本
Hardware version
硬件件版本

Node type 节点类型   NODE_DEVICE_Count 设备数量     //***wangssmm20160913    
0x00 - switch              1~4
0x01 - socket               1
0x02 - curtain              1
0x03 - lights               1
NodeValue
  节点设备状态 低4位本地设备状态值，第5位夜灯状态值 ，第七位过零信号
  
||**********************同步网关时间*****************************||
opcode||+LEN||+mac地址(6)||+Time(7)||           //***wangssmm20160913 8-->6
opcode
协议头:0x31
LEN
消息包长度	
mac地址(6)
主机网络物理地址
Time:
	年(2)-月(1)-日(1)-时(1)-分(1)-秒(1)

||**********************同步网关时间应答*****************************||
opcode||+LEN||+mac地址(6)||+value                     //***wangssmm20160913 8-->6
opcode
协议头:0x32
LEN
消息包长度	
mac地址(6)
主机网络物理地址
value
0x01 时间同步成功
0x02 时间同步失败
  
||***************************************************************||
主(从)机重启 
opcode||+LEN||+mac地址(6)||                     //***wangssmm20160913 8-->6
opcode 
协议头：0x33
LEN
消息包长度
mac地址
主机网络物理地址	

||***************************************************************||
主(从)机IP设置
opcode||+LEN||+mac地址(6)||IP（4）             //***wangssmm20160913 8-->6

opcode
协议头：0x34
LEN
消息包长度
mac地址
主机网络物理地址	
IP
网络IP

||***************************************************************||
主(从)机IP设置应答
opcode||+LEN||+mac地址(6)||+IP（4）||+value||       //***wangssmm20160913 8-->6
opcode
协议头：0x35
LEN
消息包长度
mac地址
主机网络物理地址	
IP
网络IP
value
0x01 IP地址修改成功
0x02 IP地址修改失败
||***************************************************************||
主(从)用户/密码设置(密码重设)（添加旧的密码20160918）
opcode||+LEN||+mac地址(6)||+ Net new PassWord(16)||+Net old password||    //***wangssmm20160913 8-->6  

opcode
协议头：0x36
LEN
消息包长度
mac地址
 主机网络物理地址
Net new PassWord
 新的登陆密码
Net old PassWord
 旧的登陆密码 

||***************************************************************||
主(从)用户/密码设置应答
opcode||+LEN||+mac地址(6)||+Value||             //***wangssmm20160913 8-->6

opcode
协议头：0x37
LEN
消息包长度
mac地址
主机网络物理地址
Value
0x01   主(从)用户/密码设置成功
0x02   主(从)用户/密码设置失败

||****************************************************************||
主机节点信息备份请求命令

opcode||+LEN||+mac地址(6)||                                   //***wangssmm20160913 8-->6
opcode
协议头:0x38
LEN
消息包长度	
mac地址(6)
主机网络物理地址	

||*************************************||
主机节点信息上传
||opcode||+LEN||+Node data(200)||

opcode
协议头:0x39
LEN
消息包长度
Node data
  注册信息

||**************************************||
登录主机
||opcode||+LEN||+mac地址(6)||+ Net PassWord(16)||
opcode
协议头：0x3a
LEN
消息包长度
mac
主机物理地址
Net PassWord
登陆密码

||*********************************************||
登陆主机信息响应
||opcode||+LEN||+mac地址(6)||+Landing status||
opcode:
  协议头：0x3b	
LEN :
	消息包长度
mac
 主机物理地址
Landing status
  0x01 登陆成功
  0x02 登陆失败 
  	
||**********************************************|| 
添加子账户
||opcode||+LEN||+Mac(6)||+Sub account||+password(16)||
opcode:
	协议头：0x3c
LEN 
  消息包长度
mac
  主机物理地址
Sub account
  子账户：手机号码
password
  登陆密码
    
||**********************************************||
添加子账户响应
||opcode||+LEN ||+mac(6)||+Sub account||+Set status||

opcode:
	协议头：0x3d
LEN:
  消息包长度
mac(6)
  主机物理地址
Sub account
  子账户  
Set status
  0x01 添加成功
  0x02 添加失败
||***********************************************************||
子账户密码重设
||opcode||+LEN||+Mac(6)||+Sub account||+new password(16)||+old password|| 

opcode:
	协议头：0x3E
LEN 
  消息包长度
mac
  主机物理地址
Sub account
  子账户：手机号码
new password
  登陆密码 
old password
  旧登陆密码  
 
||*********************************************************************||
子账户密码重设响应
||opcode||+LEN ||+mac(6)||+Sub account||+Set status||

opcode:
	协议头：0x3F
LEN:
  消息包长度
mac(6)
  主机物理地址
Sub account
  子账户  
Set status
  0x01 添加成功
  0x02 添加失败

||**********************************************************************||
删除子账户
||opcode||+LEN||+Mac(6)||+Sub account||+password(16)||
opcode:
	协议头：0x40
LEN 
  消息包长度
mac
  主机物理地址
Sub account
  子账户：手机号码
password
  登陆密码
  
||**********************************************************************||
删除子账户响应

||opcode||+LEN ||+mac(6)||+Sub account||+delet status||

opcode:
	协议头：0x41
LEN:
  消息包长度
mac(6)
  主机物理地址
Sub account
  子账户  
Set status
  0x01 删除成功
  0x02 删除失败 


  
||**********************************************||
添加主账户
||opcode||+LEN||+Mac(6)||+Main account||+password(16)||
opcode:
	协议头：0x42
LEN 
  消息包长度
mac
  主机物理地址
Main account
  子账户：手机号码
password
  登陆密码
    
||**********************************************||
添加主账户响应
||opcode||+LEN ||+mac(6)||+Main account||+Set status||

opcode:
	协议头：0x43
LEN:
  消息包长度
mac(6)
  主机物理地址
Main account
  子账户  
Set status
  0x01 添加成功
  0x02 添加失败
||***********************************************************||
主账户密码重设
||opcode||+LEN||+Mac(6)||+Main account||+new password(16)||+old password|| 

opcode:
	协议头：0x44
LEN 
  消息包长度
mac
  主机物理地址
Main account
  子账户：手机号码
new password
  登陆密码 
old password
  旧登陆密码  
 
||*********************************************************************||
主账户密码重设响应
||opcode||+LEN ||+mac(6)||+Main account||+Set status||

opcode:
	协议头：0x45
LEN:
  消息包长度
mac(6)
  主机物理地址
Main account
  子账户  
Set status
  0x01 添加成功
  0x02 添加失败

||**********************************************************************||
删除主账户
||opcode||+LEN||+Mac(6)||+Main account||+password(16)||
opcode:
	协议头：0x46
LEN 
  消息包长度
mac
  主机物理地址
Sub account
  子账户：手机号码
password
  登陆密码
  
||**********************************************************************||
删除主账户响应

||opcode||+LEN ||+mac(6)||+Main account||+delet status||

opcode:
	协议头：0x47
LEN:
  消息包长度
mac(6)
  主机物理地址
Main account
  子账户  
Set status
  0x01 删除成功
  0x02 删除失败 



||***********************************************************************|| 
账户登录（连接云服务器）
||opcode||+LEN||+Mac(6)||+account||+password(16)||

opcode:
	协议头：0x48
LEN：
  消息包长度
account:
	账户(手机号)
password
  登陆密码
  
||**********************************************************************||
账户登录响应（连接云服务器）
||opcode||+LEN||+Mac(6)||+account||+Connection status||

opcode:
	协议头：0x49
LEN：
  消息包长度
account:
	账户(手机号)
Connection status
  0x01 登陆成功 	
	0x02 登陆失败
	
||***********************************************************************||
获取软件/硬件版本请求消息
||opcode(1)||+LEN||+Node type(1)||+Node mac(2)||

opcode
 协议头：0x4A
LEN
 消息包长度 
Node mac:
 节点mac地址
Node type
 节点类型：0x00 - switch
           0x01 - socket
	         0x02 - curtain
	         0x03 - lights 	
			 
传感器使能反馈
||opcode||+LEN||+Node type(1)||+Node mac(2)||+Node Enable Stauts||  	
opcode 
  协议头：0x4B
LEN
  消息包长度
Node mac(2)
   节点mac地址
Node Enable Stauts（2）
   节点各个传感器使能状态：0 未使能，1 使能，每位依次为 第一个字节（0~7）：过零检测，人体感应，光感检测，小夜灯，地震监测，温度，手势    
                                                        第二个字节：   0，蜂鸣器     
	 






	
	
