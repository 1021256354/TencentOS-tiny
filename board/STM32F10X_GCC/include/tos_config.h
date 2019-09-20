 #ifndef TOS_CONFIG_H
#define  TOS_CONFIG_H

#include "stm32f10x.h"	
#include <string.h>
// Ŀ��оƬͷ�ļ����û���Ҫ�����������

#define TOS_CFG_TASK_PRIO_MAX           10u			// ����TencentOS tinyĬ��֧�ֵ�������ȼ�����

#define TOS_CFG_ROUND_ROBIN_EN          1u				// ����TencentOS tiny���ں��Ƿ���ʱ��Ƭ��ת

#define TOS_CFG_OBJECT_VERIFY           0u			// ����TencentOS tiny�Ƿ�У��ָ��Ϸ�

#define TOS_CFG_EVENT_EN                1u				// TencentOS tiny �¼�ģ�鹦�ܺ�

#define TOS_CFG_MMBLK_EN                1u   		//����TencentOS tiny�Ƿ����ڴ�����ģ��

#define TOS_CFG_MMHEAP_EN               1u		// ����TencentOS tiny�Ƿ�����̬�ڴ�ģ��

#define TOS_CFG_MMHEAP_POOL_SIZE        0x100	// ����TencentOS tiny��̬�ڴ�ش�С

#define TOS_CFG_MUTEX_EN                1u		// ����TencentOS tiny�Ƿ���������ģ��

#define TOS_CFG_QUEUE_EN                1u		// ����TencentOS tiny�Ƿ�������ģ��

#define TOS_CFG_TIMER_EN                1u		// ����TencentOS tiny�Ƿ��������ʱ��ģ��

#define TOS_CFG_SEM_EN                  1u		// ����TencentOS tiny�Ƿ����ź���ģ��

#define TOS_CFG_TICKLESS_EN             0u   // ����Tickless �͹���ģ�鿪��

#if (TOS_CFG_QUEUE_EN > 0u)
#define TOS_CFG_MSG_EN     1u
#else
#define TOS_CFG_MSG_EN     0u
#endif

#define TOS_CFG_MSG_POOL_SIZE           10u		// ����TencentOS tiny��Ϣ���д�С

#define TOS_CFG_IDLE_TASK_STK_SIZE      128u		// ����TencentOS tiny��������ջ��С

#define TOS_CFG_CPU_TICK_PER_SECOND     1000u	// ����TencentOS tiny��tickƵ��

#define TOS_CFG_CPU_CLOCK     (SystemCoreClock)	// ����TencentOS tiny CPUƵ��

#define TOS_CFG_TIMER_AS_PROC           1u		// �����Ƿ�TIMER���óɺ���ģʽ

#endif



