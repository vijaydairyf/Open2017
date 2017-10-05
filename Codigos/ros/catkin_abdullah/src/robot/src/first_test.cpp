#include "ros/ros.h"
#include "arduino_msgs/StampedUint8.h"
#include "arduino_msgs/StampedUint32.h"
#include "arduino_msgs/StampedUint64.h"
#include "arduino_msgs/StampedInt8.h"
#include "arduino_msgs/StampedInt32.h"
#include "arduino_msgs/StampedInt64.h"
#include "arduino_msgs/StampedBool.h"
#include "arduino_msgs/StampedChar.h"
#include "arduino_msgs/StampedFloat32.h"
#include "arduino_msgs/StampedFloat64.h"
#include "arduino_msgs/StampedString.h"
#include <sstream>
#include <math.h>
#include <vector>

using namespace std;

#define X 0
#define Y 1
#define A 30.0f
#define PI 3.14159265f

#define ACABOU_GIRO 1
#define ACABOU_ATUAL 2


#define GIRA 300
#define VEL_REF_DIR 301
#define VEL_REF_ESQ 302

/*---------------------------------definicoes dos sensores US-------------------------------------*/

	#define US1 0
	#define US2 1
	#define US3 2
	#define US4 3
	#define US5 4
	#define US6 5
	#define US7 6
	#define US8 7
	#define US_MAX_DIST 200
	#define NUM_IDEN_US 100
	#define QUANTIDADE_SENSOR_US 8
	#define TAMANHO_MEDIANA 5
	#define VALOR_MEDIANA 2
	//sinal chega do arduino com o id(USn) = (n-1) + NUM_IDEN_US... exemplo: id do US5 = (5-1) + 100 = 104

	#define alfa_US 0.8f
	//vector<float> sonar(QUANTIDADE_SENSOR_US);
	struct Us{
  		float valor;
  		float valores[TAMANHO_MEDIANA];
  		long long int vezes_lido;
	};

	vector<Us> ultrassom(QUANTIDADE_SENSOR_US);


/*-----------------------------------------------------------------------------------------------*/

/*----------------------------definicoes dos sensores de TOQUE-----------------------------------*/

#define TOQUE1 0
#define TOQUE2 1
#define TOQUE3 2
#define TOQUE4 3
#define TOQUE5 4
#define TOQUE6 5
#define TOQUE7 6

#define TOQUE8 7
#define TOQUE9 8

#define NUM_IDEN_TOQUE 200
#define QUANTIDADE_SENSOR_TOQUE 9

vector<bool> toque(QUANTIDADE_SENSOR_TOQUE, false);

/*-----------------------------------------------------------------------------------------------*/


template<typename ItemType>
unsigned Partition(ItemType* array, unsigned f, unsigned l, ItemType pivot)
{
    unsigned i = f-1, j = l+1;
    while(true)
    {
        while(pivot < array[--j]);
        while(array[++i] < pivot);
        if(i<j)
        {
            ItemType tmp = array[i];
            array[i] = array[j];
            array[j] = tmp;
        }
        else
            return j;
    }
}

template<typename ItemType>
void QuickSortImpl(ItemType* array, unsigned f, unsigned l)
{
    while(f < l)
    {
        unsigned m = Partition(array, f, l, array[f]);
        QuickSortImpl(array, f, m);
        f = m+1;
    }
}

template<typename ItemType>
void QuickSort(ItemType* array, unsigned size)
{
    QuickSortImpl(array, 0, size-1);
}

bool ehSonar(int id)
{
	id = id - NUM_IDEN_US;
	if (id < QUANTIDADE_SENSOR_US && id >= 0)	 return true;
	else	return false;
}

bool ehToque(int id)
{
	id = id - NUM_IDEN_TOQUE;
	if (id < QUANTIDADE_SENSOR_TOQUE && id >= 0)	 return true;
	else	return false;
}

void messageMInt64Cb( const arduino_msgs::StampedInt64& aM_int64_msg)
{
	if(ehSonar(aM_int64_msg.id))
	{
		int usPos = aM_int64_msg.id - NUM_IDEN_US; 
		int valor ;
		valor = (aM_int64_msg.data <= 0.1) ? US_MAX_DIST : aM_int64_msg.data;	
		//sonar[aN_int32_msg.id - NUM_IDEN_US] = valor * alfa_US + sonar[aN_int32_msg.id - NUM_IDEN_US] * (1 - alfa_US);
		float valorf = (float)valor ; 
		long long int iterator = ultrassom[usPos].vezes_lido ;
	    ultrassom[usPos].valores[iterator % TAMANHO_MEDIANA] = valorf;
	    float valores[TAMANHO_MEDIANA];
	    for(unsigned i = 0; i < TAMANHO_MEDIANA; ++i)
	      	valores[i] = ultrassom[usPos].valores[i];
	    QuickSort(valores,TAMANHO_MEDIANA);
	    //ultrassom[usPos].value = values[MEDIAN_VALUE];
	    valorf = valores[VALOR_MEDIANA];
	    ultrassom[usPos].valor = valorf * alfa_US + ultrassom[usPos].valor * ( 1- alfa_US);
	    ultrassom[usPos].vezes_lido++;
	}else if (ehToque(aM_int64_msg.id)) 
		toque[aM_int64_msg.id - NUM_IDEN_TOQUE] = aM_int64_msg.data;
}


void messageMFloat64Cb( const arduino_msgs::StampedFloat64& aM_float64_msg)
{
	
}

void messageMFloat32Cb( const arduino_msgs::StampedFloat32& aM_float32_msg)
{
	
}

void messageNInt32Cb( const arduino_msgs::StampedInt32& aN_int32_msg)
{
	
}

void messageNFloat32Cb( const arduino_msgs::StampedFloat32& aN_float32_msg)
{

}

float DeGrausParaRadianos(float angulo)
{
	return (angulo*PI)/180;
}

float DeRadianosParaDegraus(float angulo)
{
	return (angulo*180)/PI;
}

vector<float> AnaliseLugarDaVaca(float x1, float y1, float x2, float y2, float theta)
{
	float  distancia_direta, centro_da_entrada[2];
	vector<float> ponto_do_viro(2);
	centro_da_entrada[X] = ((x2-x1)/2);
	centro_da_entrada[Y] = ((y2-y1)/2);
	distancia_direta = sqrt(pow(centro_da_entrada[X],2) + pow(centro_da_entrada[Y],2));
	if (distancia_direta<A)
		///se ajeita (dar re e ganhar espaco para chegar melhor na vaca) e faz de novo o processo
	ponto_do_viro[X] = centro_da_entrada[X] - (A*cos(DeGrausParaRadianos(90-theta)));
	ponto_do_viro[Y] = centro_da_entrada[Y] - (A*sin(DeGrausParaRadianos(90-theta)));
	return ponto_do_viro;
}

void TrajetoriaSuaveAteAVaca(float x1, float y1, float x2, float y2, float theta)
{
	float alfa, distancia_ate_ponto_do_giro;
	vector<float> v = AnaliseLugarDaVaca(x1, y1, x2, y2, theta);
	alfa = DeRadianosParaDegraus(atan2(v[X],v[Y]));
	
	//gira(alfa)
	distancia_ate_ponto_do_giro = sqrt(pow(v[X],2) + pow(v[Y],2));
	//anda(distancia_ate_ponto_do_giro)
	//gira(-(alfa+theta))
	//anda(A)
}



int main(int argc, char **argv)
{
	ros::init(argc, argv, "first_test");
	ros::NodeHandle nh;
	/*-------------------------------------------settar as paradas do arduino mega----------------------------------------*/
	ros::Publisher pubM_int64 = nh.advertise<arduino_msgs::StampedInt64>("raspberryM_int64", 1000);
	ros::Publisher pubM_float64 = nh.advertise<arduino_msgs::StampedFloat64>("raspberryM_float64", 1000);
	ros::Publisher pubM_float32 = nh.advertise<arduino_msgs::StampedFloat32>("raspberryM_float32", 1000);
	ros::Subscriber subM_int64 = nh.subscribe("arduinoM_int64", 1000, messageMInt64Cb);
	ros::Subscriber subM_float64 = nh.subscribe("arduinoM_float32", 1000, messageMFloat64Cb);
	ros::Subscriber subM_float32 = nh.subscribe("arduinoM_float64", 1000, messageMFloat32Cb);
	arduino_msgs::StampedInt64 int64M_msg;	
	arduino_msgs::StampedFloat64 float64M_msg;
	arduino_msgs::StampedFloat32 float32M_msg;
	/*-------------------------------------------settar as paradas do arduino uno---------------------------------------*/
	ros::Publisher pubN_int32 = nh.advertise<arduino_msgs::StampedInt32>("raspberryN_int32", 1000);
	ros::Publisher pubN_float32 = nh.advertise<arduino_msgs::StampedFloat32>("raspberryN_float32", 1000);
	ros::Subscriber subN_int32 = nh.subscribe("arduinoN_int32", 1000, messageNInt32Cb);
	ros::Subscriber subN_float32 = nh.subscribe("arduinoN_float32", 1000, messageNFloat32Cb);
	arduino_msgs::StampedInt32 int32N_msg;	
	arduino_msgs::StampedFloat32 float32N_msg;
	float32N_msg.id = VEL_REF_DIR;
	float32N_msg.data = 0.8;
	ros::Rate loop_rate(10);	
	int count = 0;
	while (ros::ok())
	{
		/*int64M_msg.data = count;
		float32M_msg.data = count;
		float64M_msg.data = count;
		int32N_msg.data = count;
		float32N_msg.data = count;
		pubM_int64.publish(int64M_msg);
		pubM_float64.publish(float64M_msg);
		pubM_float32.publish(float32M_msg);
		pubN_int32.publish(int32N_msg);
		pubN_float32.publish(float32N_msg);*/
		if(count==30){
			pubN_float32.publish(float32N_msg);
			float32N_msg.id = VEL_REF_ESQ;
			pubN_float32.publish(float32N_msg);
			float32N_msg.id = VEL_REF_DIR;
			pubN_float32.publish(float32N_msg);
		}else if(count == 85){
			float32N_msg.data = 0.0;
			pubN_float32.publish(float32N_msg);
			float32N_msg.id = VEL_REF_ESQ;
			pubN_float32.publish(float32N_msg);
			float32N_msg.id = VEL_REF_DIR;
			pubN_float32.publish(float32N_msg);
			int32N_msg.id = GIRA;
			int32N_msg.data = 90;
			pubN_int32.publish(int32N_msg);
		}	
		ros::spinOnce();
		loop_rate.sleep();
		++count;
	}
	return 0;
}
