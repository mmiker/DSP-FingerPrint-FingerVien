

#include "bsp_header\z_bsp.h"

GPIO_Handle hGpio;

uint8_t		key_on = 0 ;
uint8_t		is_touch_on = 0 ;
uint8_t		interrupt_flag ;
uint8_t		Is_CMOS_Write_End ; //CMOS��ʼ����ʼʱ��0������ʱ��1.��ʼ��ʱ���е�CMOS��ʼ������ִ�У�
                                //�м���ò�Ҫ����������ʼ������
uint8_t		Is_CMOS_Save_Image = 0 ; //��Ҫ����ͼƬ�����ࣩʱ����λ�ñ���(��ʱ��ʵ��)

/**********************************************************
*��ʼ������֧�ֿ������оƬ������Һ����
***********************************************************/
void bsp(void)
{
	CSL_init();     //��ʼ������
	CSL_cfgInit() ; //��������
	gpio_init() ;  //GPIO��ʼ��
	interrupt_init (); //�жϳ�ʼ��
	
	idt72v36110_init() ; //FIFO��ʼ����ȫ�ָ�λ
	
//	cmos_CF400G_init() ; //ָ��ͷCF400G��ʼ��
//	cmos_D0307_init() ;  //ָ��ͷD0307 ��ʼ��
	cmos_OV7670_init() ; //���Ӿ�ͷOV7670��ʼ��

	ZLG7290_I2C_Init () ; //������ʼ��
	ds1302_init() ;	
	flash_init() ;
	oLCD_Init() ;
	iLCD_Init() ;
	ad7843_init() ;   //��������ʼ��
	gt23l32s4w_init() ;  //�ֿ��ʼ��
	

}
/*===================�жϳ�ʼ��=============*/
//�ⲿ�ж�6		-->		ָ��ͷI2C��ʼ��
//�ⲿ�ж�4		-->		�����ж�
void interrupt_init (void)
{
	IRQ_globalEnable();

	IRQ_clear(IRQ_EVT_EXTINT6);	//����жϱ�־
	IRQ_clear(IRQ_EVT_EXTINT4);	
	IRQ_clear(IRQ_EVT_EXTINT5);	
	
	IRQ_enable(IRQ_EVT_EXTINT6);//ʹ���ж�
	IRQ_enable(IRQ_EVT_EXTINT4);
	IRQ_enable(IRQ_EVT_EXTINT5);
	
}
/*===================�ⲿ�ж�4��������=============*/
//�����жϺ���
void External_Interrupt4 (void)
{
	IRQ_disable(IRQ_EVT_EXTINT4);//��ֹ�ж�	
	key_on = 1 ;
	IRQ_clear(IRQ_EVT_EXTINT4);//����жϱ�־
	IRQ_enable(IRQ_EVT_EXTINT4);//ʹ���ж�

}
/*===================�ⲿ�ж�5��������=============*/
//�������жϺ���
void External_Interrupt5()
{
	IRQ_disable(IRQ_EVT_EXTINT5);//��ֹ�ж�	
	is_touch_on = 1 ;
	IRQ_clear(IRQ_EVT_EXTINT5);//����жϱ�־
	IRQ_enable(IRQ_EVT_EXTINT5);//ʹ���ж�
}
/*===================�ⲿ�ж�6��������=============*/
//�ⲿ�ж�6Ϊ�����жϣ�
//1��ָ��ͷCMOS��I2C��ʼ���ж�
//2�����Ӿ�ͷCMOS��I2C��ʼ���ж�
//3�����Ӿ�ͷÿ֡����ʱ�ж�
//���� 1��2 ���ж���Ĵ�������ֻ����ϵͳ��ʼ��ʱ���У���ʼ��������ȫ���жϽ���3��
//CMOS�ڳ�ʼ��֮ǰҪ��Is_CMOS_Write_End��0����ʼ����Ҫ��Is_CMOS_Write_End��1��
void External_Interrupt6 (void)
{
	
	uint32_t i ;
	interrupt_flag = 1 ; //CMOS��I2C��ʼ���жϱ�־
	
//����Ϊʵʱ��ʾ���ֺ����๦�ܿ���
	if ( Is_CMOS_Write_End & ( !Is_CMOS_Save_Image ) ){
		LCD_SET ; //DSP׼����LCDд���� ����Һ����� Ϊʵʱ��ʾ��׼��  
		    
		iLCD_WriteReg(0x0050,0x00);//ˮƽ GRAM��ʼλ��
	    iLCD_WriteReg(0x0051,239);//ˮƽGRAM��ֹλ��
	    iLCD_WriteReg(0x0052,0x00);//��ֱGRAM��ʼλ��
	    iLCD_WriteReg(0x0053,319);//��ֱGRAM��ֹλ�� 
		iLCD_SetCursor( 0 ,0 ) ;
	    iLCD_WriteStart() ;//
		LCD_RT_DISPLAY ; //��ʼʵʱ��ʾ
	}
	if ( Is_CMOS_Save_Image ) { //���� ��ͼƬ
		LCD_SAVE_IMAGE ;		
		_delay_us(1) ; //��ʱ���ȴ�CPLD׼����
		for ( i=0 ; i<320*240 ; i++ ) {	//��ȡ����
			*( uint8_t * ) ( 0x80000000 + i ) = AL422B_DATA ;
//			*( uint8_t * ) ( 0x80100000 ) = AL422B_DATA ;
		}
		
		Is_CMOS_Save_Image = 0 ;

 		LCD_SAVE_TO_RT ; //CPLD��BED������תΪ���루�˾�����У�
		LCD_RT_DISPLAY ; //��ʼʵʱ��ʾ
	}
//����Ϊʵʱ��ʾ���ֺ����๦�ܿ���
}
/*===========GPIO��ʼ��===============*/
//ZLG7290_SDA-->GP10	����ʼ�������
//ZLG7290_SCL-->GP11	�������
//ZLG7290_INT-->GP4     ���ж����룬�½�����Ч��
//CPLD����ָ��ͷI2C��ʼ��ʱ�ж����-->GP6		�����룬��������Ч��
//DS1302_CLK-->GP9		����ʼ�������
//DS1302_IO -->GP0		����ʼ�������
//DS1302_RST-->GP3		����ʼ�������
//�������ж�-->GP5       (�ж����룬��������Ч)
void gpio_init(void)
{
	hGpio=GPIO_open(GPIO_DEV0,GPIO_OPEN_RESET);
	GPIO_configArgs(
		hGpio,//GPIO_Handle hGpio,
		0x00000000,//Uint32 gpgc,
		0x00000279,//Uint32 gpen,
		0x00000209,//Uint32 gpdir,
		0x00000000,//Uint32 gpval,//����ֵ�����ֵ
		0x00000000,//Uint32 gphm,
		0x00000000,//Uint32 gplm,
		0x00000030 //Uint32 gppol //INT4�½��ش���//INT5�½��ش���
	);
}
