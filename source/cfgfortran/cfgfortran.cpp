#include <QDebug>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QByteArray>
#include <QTextCodec>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>

#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <map>

#include <sfdfortran.h>

// ./cfgfortran 2>eee.txt
// видимо qDebug() использует sdt_err


/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
// перечень отработанных файлов исходных данных СФД-фортран версии
// файл 'b'    - исходная блочная модель (основной файл)
// файл 'str'  - описатель свойств блока обработки (точнее входных аргументов case() )
// файл 'i'    - перечень уникальных переменных в блочной модели
// файл 'mc'   - матрица связей (производный от 'b' и 'i' )
// файл 'eq'   - наименование уравнений модели, но по сути это количество блоков
// файл 'mda'  - исходные данные автономных испытаний агрегатов двигателя
// файл 'mdi'  - данные автономных испытаний агрегатов, приведенные к размерности мат.модели+связи с переменными блочн.модели
// файл 'pc_1' - Связка датчика (как измеряемого параметра) с переменными.
// подробное описание - внизу файла
/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/


QFile BlkModel( FILENAME_SFD_BLOCK_MODEL ); // файл 'b'
QFile varbdFile( FILENAME_SFD_VAR_BD );     // sfd_var_bd.txt
QFile mdiFile( FILENAME_SFD_MDI );          // файл 'mdi'
QFile mdaFile( FILENAME_SFD_MDA );          // файл 'mda'

QJsonObject jSfdConfig;
QJsonObject jRenewBlockModel;
QJsonObject jAllEquations;

QTextCodec *codec = QTextCodec::codecForName("IBM 866");

QList <TVariableGuide> VarGuide;

QList <EUnitInfo> UnitsInfo;

QStringList Variables;

QList <SFDBlock> SFDBlocks;

QList <TRenewVariable> RenewVariables;

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
// вектор функций F(X) function vectors (или число уравнений по файлу str)
// наименование уравнений модели

/*---------------------------------------------------------------------------------------------*/
// правила для образования характеристик автономных испытаний агрегатов
/*---------------------------------------------------------------------------------------------*/
UnitConvertionRule MdiUnitRule[]={
{ "BPUFuel",         4, "PressureСharact",       "h1 бнг",         CHTYPE_POLINOM, 0, { "h1 бнг",   "h2 бнг",   "h3 бнг",   "h4 бнг", "h5 бнг" }   }, /*!! h5 бнг*/
{ "PumpFuelOneTwo",  2, "TotalMoment",           "m1 нг1+2",       CHTYPE_POLINOM, 0, { "m1 нг1+2", "m2 нг1+2", "m3 нг1+2", "m4 нг1+2"} },
{ "PumpFuelOneSt",   5, "PressureСharact",       "h1 нг1",         CHTYPE_POLINOM, 0, { "h1 нг1",   "h2 нг1",   "h3 нг1",   "h4 нг1", "h5 нг1"}   }, // !!! h5 нг1
{ "PumpFuelTwoSt",   6, "PressureСharact",       "h1 нг2",         CHTYPE_POLINOM, 0, { "h1 нг2",   "h2 нг2",   "h3 нг2",   "h4 нг2", "h5 нг2"}   }, // !!! h5 нг2 - есть в модели, но нет в mdi
{ "FuelChoke",       8, "DockingAngle",          "AL др ст",       CHTYPE_NUMBER,  0, { "AL др ст"} },
{ "FuelChoke",       9, "FPDСharacter",          "a1 др",          CHTYPE_POLINOM, 1, { "np др", "a1 др",    "a2 др", "a3 др", "a4 др", "a5 др", "a6 др", "a7 др", "a8 др"} },
{ "FuelChoke",      11, "AngleMax",              "AL др макс",     CHTYPE_NUMBER,  0, { "AL др макс"} },
{ "Chamber",        10, "NomFuelConsumption",    "G ном г зав",    CHTYPE_NUMBER,  0, { "G ном г зав"} },
{ "Chamber",        12, "DiffPresMixingHead",    "dP ном г сг к",  CHTYPE_NUMBER,  0, { "dP ном г сг к"}  },
{ "Chamber",        12, "DiffPresCoolPath",      "dP ном г охл к", CHTYPE_NUMBER,  0, { "dP ном г охл к"} },
{ "Chamber",        17, "NozzleCriticalSection", "F к",            CHTYPE_NUMBER,  0, { "F к"} },
{ "BPUOxi",         14, "PressureСharact",       "h1  бно",        CHTYPE_POLINOM, 0, { "h1  бно",  "h2  бно",   "h3  бно",  "h4  бно", "h5  бно"}   },
{ "PumpOxi",         3, "Moment",                "m1 но",          CHTYPE_POLINOM, 0, { "m1 но",    "m2 но",     "m3 но",    "m4 но"}    },
{ "PumpOxi",        15, "PressureСharact",       "h1  но",         CHTYPE_POLINOM, 0, { "h1  но",   "h2  но",    "h3  но",   "h4  но", "h5  но"}    },
{ "GasGen",         16, "DiffPresOxi",           "dP ном ок гг",   CHTYPE_NUMBER,  0, { "dP ном ок гг"} },
{ "GasGen",          7, "DiffPresFuel",          "dP ном г сг гг", CHTYPE_NUMBER,  0, { "dP ном г сг гг"} },
{ "TPumpUnit",      18, "TurbineReactivity",     "e1 т тна",       CHTYPE_NUMBER,  0, { "e1 т тна",  "e2 т тна",  "e3 т тна", "e4 т тна", "e5 т тна"}    },  // !!! e5 т тна
{ "TPumpUnit",      19, "KPDTurbine",            "ro1 т тна",      CHTYPE_POLINOM, 0, { "ro1 т тна", "ro2 т тна", "ro3 т тна","ro4 т тна", "ro5 т тна"}    }, // !!!  ro5 т тна
{ "FuelConReg",     21, "DockingAngle",          "AL рр ст",       CHTYPE_NUMBER,  0, { "AL рр ст"} },
{ "FuelConReg",     21, "AngleMax",              "AL рр макс",     CHTYPE_NUMBER,  0, { "AL рр макс"} },
{ "FuelConReg",     22, "MeteringCharact",       "a1 рр",          CHTYPE_POLINOM, 1, { "np рр", "a1 рр", "a2 рр", "a3 рр", "a4 рр", "a5 рр", "a6 рр", "a7 рр", "a8 рр"} },

/*{ "BNO", "F к2",            "Single",  {"F к2"} },*/
/*{ "DRS", "dP ном г охл к2", "Single",   {"dP ном г охл к2"} },*/
//
//
{""}
};
// booster pump OXI
// booster pump Fuel


/*---------------------------------------------------------------------------------------------*/
// справочник
/*---------------------------------------------------------------------------------------------*/
TypeUnitList EngUnitsGuide[]={
{ "GasGen",         "Газогенератор"                         },
{ "Chamber",        "Камера"                                },
{ "PumpOxi",        "Насос окислителя"                      },
{ "PumpFuelOneSt",  "Насос горючего 1-ой ступени"           }, // есть насосы 1 и 2-ой ступени
{ "PumpFuelTwoSt",  "Насос горючего 2-ой ступени"           },
{ "PumpFuelOneTwo", "Насос горючего 1 и 2-ой ступени"       }, // есть насосы 1 и 2-ой ступени
{ "TPumpUnit",      "Турбонасосный агрегат(ТНА)"            }, // !!! турбина (Turbo pump unit)
{ "FuelConReg",     "Регулятор расхода горючего"            }, // fuel consumption regulator
{ "FuelChoke",      "Дроссель горючего"                     },
{ "BPUOxi",         "Бустерный насосный агрегат окислителя" }, // Booster pump unit
{ "BPUFuel",        "Бустерный насосный агрегат горючего"   },
{""}
};

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
TVarsToCharacterRule VarsToCharacterRule[]={
{ "a1 м нг1",   3, "chrtr-am-ng1"     },
{ "a1 ут нг2",  3, "chrtr-aut-ng1"    },
{ "a0T к",      6, "chrtr-atk"        },
{ "а1 вых т",   5, "chrtr-aviht"      },
{ "e1 т бнг",   5, "chrtr-etbng"      },
{ "e1 т бно",   5, "chrtr-etbno"      },
{ "ro1 т бно",  5, "chrtr-rotbno"     },
{ "m1 бно",     4, "chrtr-mbno"       },
{ "m1 бнг",     4, "chrtr-mbng"       },
{ "m1 нг2",     4, "chrtr-mng2"       },
{ "a1Ef т тна", 5, "chrtr-a-Ef-t-tna" },
{ "a1re",       4, "chrtr-a-re" },

{""}
};


/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
// в исходном файле блочной модели некоторые строки обрезаны
// формируем правил замещения текста (в основном это касается уравнений)

TBlkModeQqSub NewSubst[] = {
{ "Момент на Т БНАО",                  26, 1, "Расход через Т БНАО"         },
{ "Момент на Т БНАО",                  26, 2, "Температура на вых Т БНАО"   },
{ "Момент на Т БНАО",                  26, 3, "Давление в зазоре Т БНАО"    },
{ "Момент на Т БНАО",                  26, 4, "Степень реактивности Т БНАО" },
{ "Момент на Т БНАО",                  26, 5, "КПД Т БНАО"                  },


{ "Момент на Т ТНА",                   55, 1, "Расход через Т ТНА"         },
{ "Момент на Т ТНА",                   55, 2, "Температура на вых Т ТНА"   },
{ "Момент на Т ТНА",                   55, 3, "Степень реактивности Т ТНА" },
{ "Момент на Т ТНА",                   55, 4, "КПД Т ТНА"                  },
{ "Момент на Т ТНА",                   55, 5, "Давление в зазоре Т ТНА"    },
{ "Момент на Т ТНА",                   55, 6, "U/Cad Т ТНА"                },

{ "Расход через Т  БНГ",               27, 1, "Момент на -------------"    },
{ "Расход через Т  БНГ",               27, 2, "КПД  Т  БНГ"                },
{ "Давление в зазоре Т на перифирии",  83, 1, "Утечка че -------------"    },

{""}
};

// "Момент на Т БНАО"
// 1946
// Расход через Т БНАО
// Температура на вых Т БНАО
// Давление в зазоре Т БНАО
// Степень реактивности Т БНАО

// "Момент на Т ТНА"
// Расход через Т ТНА
// Температура на вых Т ТНА
// Степень реактивности Т ТНА
// Давление в зазоре Т ТНА
// U/Cad Т ТНА

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
// в исходном файле блочной модели есть переменные, которые по смыслу должны быть другими
TBlkModeVarSubst BlkModeVarSubst[] = {
{ "-",        "zero"  },
{ "0.",       "zero"  },
{ "0.0",      "zero"  },
{ "ind ок1", "ind ок" },
{ "T газ вых то2", "T газ вх т бно"},
{""}
};


TTypeToSection TypeToSection[]={
{  1, true,  JSON_BOUNDARY_OUT_OF_EQUATIONS,    "Переменные, рассчитываемые вне системы уравнений"                             },
{  3, false, JSON_TECHNICAL_CONDITIONS,         "Данные из технических условий"                                                },
{  2, true,  JSON_CALC_UNKNOWN_VARIABLE,        "Переменные, рассчитываемые как неизвестные в системе уравнений"               },
{ 21, true,  JSON_MEASURED_VARIABLE,            "Переменные, рассчитываемые как неизвестные в системе уравнений и измеряемые"  },
{ 31, false, JSON_STAND_ALONE_DATA,             "Данные автономных испытаний агрегатов"                                        },
{  4, true,  JSON_CLOSED_AND_MEASURED_VARIABLE, "Измеренные замыкающие парамеры системы уравнений"                             },
{  7, true,  JSON_CONTROL_VARIABLE,             "Управляющие переменные"                                                       },
{  1, true,  JSON_CONSTANT,                     "Константы общие"                                                              },
{  8, true,  JSON_SELECT_CONSTANT,              "Константы селекции"                                                           },
{  9, true,  JSON_EXPERT_AMENDMENTS,            "Экспертные поправки"                                                          },
{  0 }, //end
};

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
int UsedCase[ MAX_SFD_CASE_ID+10 ]; // для расчета статистики используемых case()

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
QMap <QString,QString> RusToEng;
/* = {
{'a':'4'},
{'б':'5'}
};*/

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
void InitRusToEng()
{
   RusToEng.insert("а", "a");
   RusToEng.insert("б", "b");
   RusToEng.insert("в", "v");
   RusToEng.insert("г", "g");
   RusToEng.insert("д", "d");
   RusToEng.insert("е", "e");
   RusToEng.insert("ё", "iou");
   RusToEng.insert("ж", "zh");
   RusToEng.insert("з", "z");
   RusToEng.insert("и", "i"); //
   RusToEng.insert("й", "ij");
   RusToEng.insert("к", "k");
   RusToEng.insert("л", "l");
   RusToEng.insert("м", "m");
   RusToEng.insert("н", "n");
   RusToEng.insert("о", "o");
   RusToEng.insert("п", "p");
   RusToEng.insert("р", "r");
   RusToEng.insert("с", "s");
   RusToEng.insert("т", "t"); //
   RusToEng.insert("у", "u");
   RusToEng.insert("ф", "f");
   RusToEng.insert("х", "h");
   RusToEng.insert("ц", "c");
   RusToEng.insert("ч", "ch");
   RusToEng.insert("ш", "sh");
   RusToEng.insert("щ", "shh");
   RusToEng.insert("ъ", "tz1");
   RusToEng.insert("ы", "i1i");
   RusToEng.insert("ь", "mz1"); //
   RusToEng.insert("э", "ae");
   RusToEng.insert("ю", "yu");
   RusToEng.insert("я", "ya");
   RusToEng.insert(" ", "-");

   qDebug() << RusToEng;
}


/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
// удаляем все пробелы и переводы строки от конца строки до первого символа
void RemoveUnwantedSymbols( QString *StrName )
{

    if( StrName->size() == 0 ) return;

    int LastPos = StrName->size()-1;
    //qDebug() << "bibi:" << *(StrName);
    //qDebug() << "bibi" << StrName->at( LastPos )<< "ii";

    while( StrName->at( LastPos ) == ' ' || StrName->at( LastPos ) == '\n' )
    {
         //qDebug() << "bibi" << StrName->at( LastPos )<< "ii";
         StrName->remove( LastPos, 1 );
         if( StrName->size() == 0 ) break;
         LastPos = StrName->size()-1;
    }

}

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
void PrepareVariableGuide()
{

    while( !varbdFile.atEnd() )
    {
        TVariableGuide OneVar;

        // считываем линию
        QByteArray RdLine;
        QString UnitLine;
        RdLine.clear();
        RdLine = varbdFile.readLine();

        //OneVar.Name.append( RdLine );
        //OneVar.Description.append( RdLine );

        //OneVar.Name.remove( POSITION_VARBD_DESCRIPTION, );

        // была небольшая проблемма с QByteArray, т.к. в редакторе вроде выглядит форматированно,
        // а в байтах все русские буквы имеют размерность в 2 байта ( т.к. UTF-8 )
        // поэтому простое решение QByteArray копируем QString и далее работем с QString
        // в которой уже работаем с символами строки (как эквивалент байта в dos-кодировке)
        UnitLine.append( RdLine );
        OneVar.OderNum     = UnitLine.mid( POSITION_VARGUIDE_ONUM,  4 );
        OneVar.Name        = UnitLine.mid( POSITION_VARGUIDE_NAME, 22 );
        OneVar.TypeVar     = UnitLine.mid( POSITION_VARGUIDE_TYPE,  2 );
        OneVar.Description = UnitLine.mid( POSITION_VARGUIDE_DESCRIPTION );

        RemoveUnwantedSymbols( &OneVar.Name        );
        RemoveUnwantedSymbols( &OneVar.Description );

        bool ConvFlg;
        OneVar.Oder   = OneVar.OderNum.toInt( &ConvFlg );
        OneVar.TypeV  = OneVar.TypeVar.toInt( &ConvFlg );

        //qDebug() << OneVar.OderNum << "   " << OneVar.Name << "   " << OneVar.Description << " Type:" << OneVar.TypeVar;
//        qDebug("%3d  %2d     %-16s  -  %s", OneVar.Oder, OneVar.TypeV, OneVar.Name.toUtf8().data(),  OneVar.Description.toUtf8().data() );

        // вывод для отправки в редактор и поиска орфогр.ошибок
//        qDebug("%3d  %s", OneVar.Oder, OneVar.Description.toUtf8().data() );


        //printf("%3d  %2d %32s  -  %s\n", OneVar.Oder, OneVar.TypeV, OneVar.Name.toUtf8().data(),  OneVar.Description.toUtf8().data() );


        VarGuide.append(OneVar);

    }



}

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
void PrintVarGuide( int TypeV )
{

    qDebug() << "---------------------------------------------------------------------------------------------";
    qDebug() << "---------------------------------------------------------------------------------------------";
    qDebug() << "Справочник (по типу): ";

    if( TypeV ==  1 ) qDebug() << "тип  1- Переменная рассчитывается вне системы уравнений";
    if( TypeV ==  2 ) qDebug() << "тип  2- Переменная рассчитывается как неизвестная в системе уравнений";
    if( TypeV == 21 ) qDebug() << "тип 21- Переменная рассчитывается как неизвестная в системе уравнений и измеряется";
    if( TypeV ==  3 ) qDebug() << "тип  3- Константа из ТУ";
    if( TypeV == 31 ) qDebug() << "тип 31- Константа из автономных испытаний";
    if( TypeV ==  4 ) qDebug() << "тип  4- Измеренные замыкающие парамеры системы уравнений";
    if( TypeV ==  6 ) qDebug() << "тип  6- Вспомогательные (необязательные)";
    if( TypeV ==  7 ) qDebug() << "тип  7- управляющие парамеры";

    qDebug() << endl;

    QList<TVariableGuide>::iterator ibIt;
    for (ibIt = VarGuide.begin(); ibIt != VarGuide.end(); ++ibIt)
    {
        TVariableGuide ib;
        ib = *ibIt;
        if( ib.TypeV == TypeV )
        {
            qDebug("%-4s   %-20s  %s", ib.OderNum.toUtf8().data(), ib.Name.toUtf8().data(), ib.Description.toUtf8().data() );
            //qDebug() << ib.Name << "" << ib.Description;
        }
    }

    qDebug() << endl;

}

/*---------------------------------------------------------------------------------------------*/
// формирование данных автономных испытаний агрегатов
// из файла 'mdi' ()
/*---------------------------------------------------------------------------------------------*/
void ReadUnitData()
{
    int lineQnty=0;

    while( !mdiFile.atEnd() )
    {
        EUnitInfo  OneUnitInfo;
        QByteArray RdLine;
        QString    VarLine;

        lineQnty++;

        // считываем линию
        RdLine.clear();
        RdLine = mdiFile.readLine();

        // первую строчку пропускаем, в ней номер двигателя
        if( lineQnty == 1 ) continue;

        // преобразуем строку в utf-8
        VarLine = codec->toUnicode( RdLine );

        OneUnitInfo.sInitValue     = VarLine.mid( POSITION_MDI_INIT_VALUE, 14 );
        OneUnitInfo.VarName        = VarLine.mid( POSITION_MDI_VAR_NAME );

        RemoveUnwantedSymbols( &OneUnitInfo.sInitValue  );
        RemoveUnwantedSymbols( &OneUnitInfo.VarName     );
        qDebug() << Variables.contains( OneUnitInfo.VarName ) << "  " << OneUnitInfo.VarName << "   " << OneUnitInfo.sInitValue;


        UnitsInfo.append( OneUnitInfo );

    }

}

/*---------------------------------------------------------------------------------------------*/
// StandAloneData
// JSON_STAND_ALONE_DATA
/*---------------------------------------------------------------------------------------------*/
void JsonUnitsData()
{
    int mdIndx=0;
    int MdiVarCnt=0;

    QJsonObject jStandAloneData;

    //QJsonArray  jUnitData;

    // сюда пока кладем все данные относящиеся к характеристике
    QString Calibrations;

    //
    QString UnitCode;

    qDebug() << "---------------------------------------------------------------------------------------------";
    qDebug() << "---------------------------------------------------------------------------------------------";
    while( MdiUnitRule[mdIndx].UnitCode != "" )
    {

         QJsonObject jUnit;
         QJsonObject SfdObject;
         QJsonArray  OldVars;

         Calibrations.clear();

         MdiVarCnt += MdiUnitRule[mdIndx].MdiVar.size();


         // обозначение агрегата
         int eugIndx=0;
         while( EngUnitsGuide[eugIndx].UnitCode != "" )
         {
             if( EngUnitsGuide[eugIndx].UnitCode ==  MdiUnitRule[mdIndx].UnitCode )
             {
                 qDebug() << EngUnitsGuide[eugIndx].Name;
                 UnitCode = MdiUnitRule[mdIndx].UnitCode;

             }
             eugIndx++;
         }
         //jUnit.insert( EngUnitsGuide[eugIndx].UnitCode, );

         // уникальный идентификатор характеристики
         QString CharactUid = QString(JSON_STAND_ALONE_DATA) + "." + MdiUnitRule[mdIndx].UnitCode + "."  + MdiUnitRule[mdIndx].CharactId;
         qDebug() << CharactUid;
         SfdObject.insert( jSfdObjectKeyId, CharactUid );

         // find description
         // находим описание переменной ( название по русски ), являющееся описанием искомой характеристики (или константы)
         QList<TVariableGuide>::iterator ibIt;
         for (ibIt = VarGuide.begin(); ibIt != VarGuide.end(); ++ibIt)
         {
             TVariableGuide ib;
             ib = *ibIt;
             if( ib.Name == MdiUnitRule[mdIndx].DescriptionVar )
             {
                 qDebug() << ib.Description;

                 SfdObject.insert( jSfdObjectKeyDecription, ib.Description );

                 //EngUnitsGuide[eugIndx].jUnit.insert(  );
                 break;
             }
         }

         SfdObject.insert( "Type", MdiUnitRule[mdIndx].Type );

         // считываем коэффициенты характеристики
         for( int vIndx=0; vIndx < MdiUnitRule[mdIndx].MdiVar.size(); vIndx++ )
         {

             // цикл по данным файла mdi
             QList<EUnitInfo>::iterator uIt;
             for( uIt = UnitsInfo.begin(); uIt != UnitsInfo.end(); ++uIt )
             {
                 EUnitInfo euInf;
                 euInf = *uIt;
                 if( euInf.VarName == MdiUnitRule[mdIndx].MdiVar[vIndx] )
                 {
                     Calibrations.append( euInf.sInitValue );
                     Calibrations.append( "   ");
                     //qDebug() << nnn.sInitValue;
                     break;
                 }
             }


            //заодно заносим старые имена переменных
            OldVars.append( MdiUnitRule[mdIndx].MdiVar[vIndx] );

         }
         SfdObject.insert( jSfdObjectKeyValue, Calibrations );
         SfdObject.insert( "OldVars", OldVars );
         MdiUnitRule[mdIndx].jOneUnitData.insert( jSfdObject, SfdObject  );


         // обозначение агрегата
         eugIndx=0;
         while( EngUnitsGuide[eugIndx].UnitCode != "" )
         {
             if( EngUnitsGuide[eugIndx].UnitCode ==  MdiUnitRule[mdIndx].UnitCode )
             {
                 qDebug() << EngUnitsGuide[eugIndx].Name;
                 // UnitCode = MdiUnitRule[Indx].UnitCode;
                 // EngUnitsGuide[eugIndx].jUnit.insert( "UnitCode", EngUnitsGuide[eugIndx].UnitCode );

                 EngUnitsGuide[eugIndx].jUnit.insert( MdiUnitRule[mdIndx].CharactId, MdiUnitRule[mdIndx].jOneUnitData );
                 EngUnitsGuide[eugIndx].jUnit.insert( jFactoryNum, "-" );
                 EngUnitsGuide[eugIndx].jUnit.insert( jNameUnit, EngUnitsGuide[eugIndx].Name );

             }
             eugIndx++;
         }





         qDebug() << MdiUnitRule[mdIndx].UnitCode;
         qDebug() << MdiUnitRule[mdIndx].MdiVar;
         qDebug() << Calibrations;
         qDebug() << ".......................................................\n";//
         mdIndx++;

         //jStandAloneData.insert(  );

    }


     int eugIndx=0;
     while( EngUnitsGuide[eugIndx].UnitCode != "" )
     {

         //jStandAloneData.insert( EngUnitsGuide[eugIndx].UnitCode,  EngUnitsGuide[eugIndx].jUnit );
         jStandAloneData.insert( EngUnitsGuide[eugIndx].Name,  EngUnitsGuide[eugIndx].jUnit );

         eugIndx++;
     }


     qDebug() << "MdiCnt = " << MdiVarCnt;

     //jSfdConfig.insert(  JSON_STAND_ALONE_DATA, jStandAloneData );
     jSfdConfig.insert(  "данные автономных испытаний агрегатов", jStandAloneData );

//     qDebug() << "new document:\n" << ((const char*)QJsonDocument(jSfdConfig).toJson(QJsonDocument::Compact));

}

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
void GroupVarsToCharacter()
{
    int Indx=0;
    int kInx=0;

    qDebug() << "---------------------------------------------------------------------------------------------";
    qDebug() << "---------------------------------------------------------------------------------------------";
    while( VarsToCharacterRule[Indx].FirstVar != "" )
    {

         kInx=0;
         // смещаемся к описанию переменной в Guide
         QList<TVariableGuide>::iterator ibIt;
         for( ibIt = VarGuide.begin(); ibIt != VarGuide.end(); ++ibIt )
         {
             QString Character;
             TVariableGuide ib;
             ib = *ibIt;

             if( ib.Name == VarsToCharacterRule[Indx].FirstVar )
             {
                 qDebug() << ib.Description;

                 for( int jIndx=0; jIndx<VarsToCharacterRule[Indx].Count; jIndx++)
                 {
                      TVariableGuide ibLk;
                      ibLk = VarGuide.at( kInx + jIndx );

                      VarsToCharacterRule[Indx].Vars.append( ibLk.Name );

                      Character.append( ibLk.Name );
                      Character.append( "   ");
                      // далее нужно по ibLk.Name найти в файле 'b'
                      // первую встречающуюся переменную и скопировать поле начальн.значение
                      // это будем делать при разборе блочной модели
                      // и установления признака замещения


                 }
                 qDebug() << VarsToCharacterRule[Indx].Vars.size()<< "   " << VarsToCharacterRule[Indx].Vars;
                 //qDebug() << Character;
                 break;
             }
             kInx++;
         }
         Indx++;
    }

}

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
void PrepareBlockModel()
{
    QStringList BlockModel;

    int  LineNum          = 0;
    int  ElementsFxQnty   = 0;
    int  EqNumber         = 0;
    int  TotalBlockQnty   = 0; // это всего блоков
    int  TotalEqQnty      = 0; // это всего блоков типа уравнение
    int  LastStringToken  = STRING_VARIABLE; // тип предыдущей строки. файл всегда начинается с нового блока, поэтому для алгоритма иницииуем как STRING_VARIABLE

    // признак нового блока
    bool FirstBlockSign=true;

    // признак типа, последней оработанной строки
    bool LastLineIsVar=true; // true: строка имеет тип переменной

    SFDBlock SfdBlk;

    qDebug() << "---------------------------------------------------------------------------------------------";
    qDebug() << "---------------------------------------------------------------------------------------------";

    while( !BlkModel.atEnd() )
    {
        // считываем линию
         QByteArray RdLine = BlkModel.readLine();

        // заменяем нулевые байты на пробелы (есть в исходном файле одно такое неприятное место )
        RdLine.replace( 0x00, 0x20 );

        // преобразуем в utf-8
        QString BlkModelLine = codec->toUnicode( RdLine );

        QString BlockCaseID;
        QString BlockName;

        QString OrdinalNum; // порядковый номер блока

        //QString variableName;
        //QString VariableInitValue;
        QString EquationQnty;

        // последняя линия может быть короткой и не содержать инфы
        // этот момент нужно доработать
        if( BlkModelLine.size() > 10 )
        {

//           qDebug() << "line: " << LineNum << "BlkLine: " << BlkLine;

            if( BlkModelLine.at( POSITION_NUM_BLOCK_TYPE_ID ).isDigit() || BlkModelLine.at( POSITION_NUM_BLOCK_ID_2SYMBOL ).isDigit() )
            {
//                 qDebug() << "----- новый блок  ------";
                   // и соответсвенно строка это описатель элемента массива F()

                   ElementsFxQnty++;
            }
            else
            {
                 //qDebug() << "переменная блока";
            }

            // читаем номер блока
            // номер case() не более чем трехзначное число. размещено в позиции 4-7
            BlockCaseID.append( BlkModelLine.at( POSITION_NUM_BLOCK_TYPE_ID     ));
            BlockCaseID.append( BlkModelLine.at( POSITION_NUM_BLOCK_TYPE_ID + 1 ));
            BlockCaseID.append( BlkModelLine.at( POSITION_NUM_BLOCK_TYPE_ID + 2 ));
            BlockCaseID.append( BlkModelLine.at( POSITION_NUM_BLOCK_TYPE_ID + 3 ));

            // читаем количество уравнений в блоке (не более чем двузначное число).
            EquationQnty.append( BlkModelLine.at( POSITION_BLOCK_EQ_QNTY   ) );
            EquationQnty.append( BlkModelLine.at( POSITION_BLOCK_EQ_QNTY+1 ) );

            // читаем порядковый номер блока (как счетчик)
            // после начала блоков с уравнениями > 0, счетчик сбрасывается
            OrdinalNum.append( BlkModelLine.at( POSITION_BLOCK_ORDINAL_NUM     ));
            OrdinalNum.append( BlkModelLine.at( POSITION_BLOCK_ORDINAL_NUM + 1 ));
            OrdinalNum.append( BlkModelLine.at( POSITION_BLOCK_ORDINAL_NUM + 2 ));
            OrdinalNum.append( BlkModelLine.at( POSITION_BLOCK_ORDINAL_NUM + 3 ));

            bool OrdinalConvFlg;
            int  OrdinalBlockNum = OrdinalNum.toInt( &OrdinalConvFlg );


            //qDebug() << "line: "<< LineNum << "  " << BlockNum;

            //если строка содержит цифры, то это описание уравнения (или начало блока)

            bool ConvCaseIDFlg;
            int CaseID = BlockCaseID.toInt( &ConvCaseIDFlg );

            // если преобразование номера блока успешно, то обрабатываемая строка - это объект F() по фортрану.
            // Если до этого элемента было описание переменной, то это начало описания нового блока case()
            if( ConvCaseIDFlg )
            {

                BlockName.append( BlkModelLine );
                // удаляем все до 15 позиции
                BlockName.remove( 0, POSITION_BLOCK_NAME );
                // удаляем все пробелы и переводы строки от конца строки до первого символа
                RemoveUnwantedSymbols( &BlockName );

                bool ConvEqQFlg;
                int EqQnty   = EquationQnty.toInt( &ConvEqQFlg );


                //BlockNumber
                if( LastStringToken == STRING_VARIABLE )
                {

                     // добавяем сформированный на предуыщих шагах блок
                     if( FirstBlockSign != true ) SFDBlocks.append(SfdBlk);

                     TotalBlockQnty++;

                     // ощищаем все предыдущие данные, т.к. у нас новый блок
                     SfdBlk.PrimaryCaseID.clear();
                     SfdBlk.PrimaryBlockName.clear();
                     SfdBlk.Description.clear();
                     SfdBlk.Equations.clear();
                     SfdBlk.NameVars.clear();
                     SfdBlk.Vars.clear();
                     SfdBlk.EqQnty      = 0;
                     SfdBlk.CaseID      = 0;
                     SfdBlk.OldOrderNum = 0;
                     EqNumber           = 0;

                     if( ConvEqQFlg )
                     {
                          TotalEqQnty += EqQnty;
                     }

                     SfdBlk.PrimaryCaseID    = BlockCaseID;
                     SfdBlk.PrimaryBlockName = BlockName;
                     SfdBlk.Description      = "Block. "+ BlockName;
                     SfdBlk.PrimaryEqQnty    = EquationQnty;
                     SfdBlk.PrimaryOrderNum  = OrdinalNum; //
                     SfdBlk.EqQnty           = EqQnty;
                     SfdBlk.CaseID           = CaseID;

                     bool cFlg;
                     SfdBlk.OldOrderNum     = SfdBlk.PrimaryOrderNum.toInt( &cFlg );
                     // первая строка, которая соответсвует началу блока, может являеться ещё и источником уравнения
                     // причем описание бока = описанию уравнения
                     // это если EqQnty != 0
                     if( EqQnty != 0 )
                          SfdBlk.Equations.append( BlockName );

                     qDebug() << SfdBlk.PrimaryCaseID << "Name: " << SfdBlk.Description;

                }
                else
                {
                     // если предыдущая строка имеет тип "блока", то обрабатываемая строка это принадлежность "открытого" блока

                     // но перед добавлением, проверяем нужно ли замещать текст

                     int iSbs=0;
                     while( NewSubst[iSbs].BlockName != "")
                     {
                         if( NewSubst[iSbs].BlockName == SfdBlk.PrimaryBlockName &&
                             NewSubst[iSbs].CaseId    == SfdBlk.CaseID           &&
                             NewSubst[iSbs].posnum    == SfdBlk.Equations.size() ) // немного корявое выражение
                         {
                             BlockName.clear();
                             BlockName = NewSubst[iSbs].NewEqTxt;

                         }
                         iSbs++;
                     }


                     SfdBlk.Equations.append( BlockName );
                }
                // делаем
                //TotalEqQnty += EqQnty;

                // qDebug() << "OrdinalNum: " << OrdinalBlockNum << "  CaseID: "<< BlockCaseID << " CaseID num:" << CaseID <<  "EqQnty: " << EquationQnty << " " << " BlockName: " << BlockName;

                LastStringToken = STRING_EQUATION;
                FirstBlockSign  = false;

            }
            else // то это переменная блока
            {

                 TVariable variable;

                 // -----> формируем имя
                 variable.Name.append( BlkModelLine );
                 // удаляем все до 15 позиции
                 variable.Name.remove( 0, POSITION_BLOCK_VARIABLE_NAME );
                 // удаляем все пробелы и переводы строки от конца строки до первого символа
                 RemoveUnwantedSymbols( &variable.Name );

                 // Если в строке есть символ '!', то удаляем все от этого символа до конца строки
                 // все от '!' восприимается как комментарий
                 int indxSimb = variable.Name.indexOf('!');
                 if( indxSimb != -1 )
                 {

                      //qDebug() << " !!!!! remove !!!!!: " << variable.Name;
                      variable.Name = variable.Name.left(indxSimb);

                      // еще раз
                      // удаляем все пробелы и переводы строки от конца строки до первого символа
                      RemoveUnwantedSymbols( &variable.Name );
                 }


                // -----> формируем порядковый номер (как счетчик)
                variable.OrdinalN.append( BlkModelLine.at( POSITION_VAR_ORDINAL_NUM     ));
                variable.OrdinalN.append( BlkModelLine.at( POSITION_VAR_ORDINAL_NUM + 1 ));
                bool OrdinalConvFlg;
                variable.OrdinalNum = variable.OrdinalN.toInt( &OrdinalConvFlg );

                 // -----> формируем начальное значение
                 variable.sInitValue.append( BlkModelLine );
                 // удаляем до начала числа
                 variable.sInitValue.remove( 0, POSITION_BLOCK_VARIABLE_INITVAL );
                 //int size = VariableInitValue.size();
                 // удаляем после текста числа (всего 12 символов)
                 //VariableInitValue.remove( 12, size );
                 variable.sInitValue.truncate( 12 );
                 // меняем символ мантиссы: "D"-это фортран, "e"-это с++
                 variable.sInitValue.replace("D","e");
                 //bool ConvFlg;
                 variable.InitValue = variable.sInitValue.toDouble( &variable.ConvFlg );

                 // ищем нужно ли заменить имя переменной
                 // хотя наверно это нужно сделать позже (т.к. это завязано на нач.значение)
                 int ivSubts=0;
                 while( BlkModeVarSubst[ivSubts].VarName != "" )
                 {
                     if( variable.Name == BlkModeVarSubst[ivSubts].VarName )
                     {
                         variable.Name = BlkModeVarSubst[ivSubts].SubstVarName;
                     }
                     ivSubts++;
                 }

                 // find description
                 // -----> находим описание переменной в Variable Guide
                 QList<TVariableGuide>::iterator i;
                 for (i = VarGuide.begin(); i != VarGuide.end(); ++i)
                 {
                     TVariableGuide nnn;
                     nnn = *i;
                     if( nnn.Name == variable.Name )
                     {
                         // копируем из guide нужные поля
                         variable.Description = nnn.Description;
                         variable.TypeV       = nnn.TypeV;

                         //qDebug() << nnn.Description;
                         break;
                     }
                 }

                 // -----> формируем признак неизвестной в системе уравнений
                 variable.UnknownVarFlg = false;
                 variable.UnknownSign.append( BlkModelLine.at( POSITION_VAR_UNKNOWN_SIGN     ));
                 if( variable.UnknownSign == "1" ) variable.UnknownVarFlg = true;

                 variable.PrintFlg = false;
                 variable.PrintSign.append( BlkModelLine.at(  POSITION_VAR_PRINT_SIGN        ));
                 if( variable.PrintSign == "1" ) variable.PrintFlg = true;


                 // -----> формируем признак SubstitutionFlg
                 // ищем по справочникам
                 // вначале ищем в данных автономных испытаний агрегатов
                 variable.Substitution = 0;
                 int iMdi = 0;
                 while( MdiUnitRule[iMdi].UnitCode != "" )
                 {
                     for( int jIndx=0; jIndx < MdiUnitRule[iMdi].MdiVar.size(); jIndx++ )
                     {

                          if( variable.Name == MdiUnitRule[iMdi].MdiVar[jIndx] )
                          {
                              variable.Substitution = 1;
                              break;
                          }

                     }
                     iMdi++;

                 }

                 // справочник характеристик
                 int nRule = 0;
                 while( VarsToCharacterRule[nRule].FirstVar != "" )
                 {

                      for( int nv=0; nv < VarsToCharacterRule[nRule].Vars.size(); nv++ )
                      {

                          if( variable.Name == VarsToCharacterRule[nRule].Vars.at(nv) )
                          {
                              variable.Substitution = 2;
                              //VarsToCharacterRule[nRule].Values.append( variable.InitValue );
                              break;
                          }

                              // далее нужно по ibLk.Name найти в файле 'b'
                              // первую встречающуюся переменную и скопировать поле начальн.значение
                              // это будем делать при разборе блочной модели
                              // и установления признака замещения
                      }
                      nRule++;

                 }


                 // %lf
                 qDebug("%3d Unk:%2d Prt:%2d %15le %26s", variable.OrdinalNum, variable.UnknownVarFlg , variable.PrintFlg, variable.InitValue, variable.Name.toUtf8().data() );

                 //qDebug() << " VariableName: " << variable.Name << "  InitV: " << variable.sInitValue << " ConvFlg: "<< variable.ConvFlg << " cnv: " << variable.InitValue << "  ordinal: " << variable.OrdinalN;
//                 qDebug() << variable.Description;

                 // добавляем к общему List
                 //Variables << variable.Name;
                 Variables.append( variable.Name );


                 SfdBlk.Vars.append( variable );

                 LastStringToken = STRING_VARIABLE;


                 // ищем совпадение в
                 //varbdFile

            }
            LineNum++;

        }
        else
        {
            // так как конец файла, то заносим последний блок
            SFDBlocks.append(SfdBlk);
            //считам что достигли конца файла
            break;
        }


        //if( ModBlock )

        BlockModel.append( BlkModelLine );

        //qDebug() << ModBlock;

        //process_line(line);
        //if( LineNum > 20 ) break;

    }

#define BLOCK_MOEL_AND_VARIABLE
#ifdef BLOCK_MOEL_AND_VARIABLE
    qDebug() << "LineNum                       = " << LineNum;
    qDebug() << "ElementsFxQnty Line           = " << ElementsFxQnty; // кол-во элементов массива F()
    qDebug() << "Var Qnty (before remove Dups) = " << Variables.size();
    qDebug() << "Control Sum                   = " << Variables.size() + ElementsFxQnty;
    qDebug() << "TotalBlockQnty ( see file 'mc' ) = " << TotalBlockQnty;
    qDebug() << "TotalEqQnty ( see file 'eq' )    = " << TotalEqQnty;
    qDebug() << "SFDBlocksQnty                    = " << SFDBlocks.count();

    //qDebug() << Variables;
#endif

    Variables.removeDuplicates();

    qDebug() << "Unique Var Qnty ( see file 'i' ) = " << Variables.size(); // see file 'i'



}

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
void PrepareRenewVariable()
{
    qDebug() << "---------------------------------------------------------------------------------------------";
    qDebug() << "---------------------------------------------------------------------------------------------";

    for( int vIndx=0; vIndx<Variables.size(); vIndx++)
    {

        TRenewVariable rVar;
        rVar.Name         = Variables.at(vIndx);
        rVar.GuideFlg     = false;
        rVar.DeleteFlg    = false;
        rVar.TypeV        = 0;
        rVar.Substitution = 0;
        rVar.ObjectType   = CHTYPE_NUMBER; // по умолчанию

        //
        // find description
        // -----> находим описание переменной в Variable Guide
        QList<TVariableGuide>::iterator itVG;
        for( itVG = VarGuide.begin(); itVG != VarGuide.end(); ++itVG )
        {
            TVariableGuide nnn;
            nnn = *itVG;
            if( nnn.Name == rVar.Name )
            {
                // копируем из guide нужные поля
                rVar.Description = nnn.Description;
                rVar.TypeV       = nnn.TypeV;
                rVar.GuideFlg    = true;

                //qDebug() << nnn.Description;
                break;
            }
        }


        // конвертируем имя в only English
        QString NewConvertName;
        for( int iSymb=0; iSymb<rVar.Name.size(); iSymb++)
        {
             QChar   Symbol;
             QString NewSymbol;
             bool  UpperFlg;
             Symbol   = rVar.Name.at( iSymb );
             UpperFlg = Symbol.isUpper();
             Symbol   = Symbol.toLower();

             if( RusToEng.contains( Symbol ) )
             {
                  NewSymbol = RusToEng.value( Symbol );
             }
             else
             {
                  NewSymbol = Symbol;
             }

             // возвращаем обратно в верхний регистр
             if( UpperFlg )
                NewSymbol = NewSymbol.toUpper();

             // QString OldSymbol = Symbol;
             //qDebug() << "conv symbol" << RusToEng.contains( Symbol ) << "  " << OldSymbol << " -> " << NewSymbol;

             NewConvertName += NewSymbol;
        }

        rVar.ConvertName = NewConvertName;



        // пробуем найти эту переменную в справочнике правил mdi
        int mdIndx=0;
        while( MdiUnitRule[mdIndx].UnitCode != "" )
        {
             for( int jIndx=0; jIndx < MdiUnitRule[mdIndx].MdiVar.size(); jIndx++ )
             {

                  if( rVar.Name == MdiUnitRule[mdIndx].MdiVar[jIndx] )
                  {
                      // если первая переменная, то вместо неё будем подставлять характеристику
                      if( jIndx == 0 ) // т.е. первая переменная
                      {
                          if( MdiUnitRule[mdIndx].MdiVar.size() > 1 ) // то это полином
                          {
                              rVar.ObjectType   = CHTYPE_POLINOM;
                              rVar.sInitValue   = "-----------";
                          }
                          rVar.Substitution = 1;
                          // уникальный идентификатор характеристики
                          rVar.SfdObjectID  = QString(JSON_STAND_ALONE_DATA) + "." + MdiUnitRule[mdIndx].UnitCode + "."  + MdiUnitRule[mdIndx].CharactId;

                      }
                      else
                      {
                          if( jIndx == 1 && MdiUnitRule[mdIndx].DegreePolynom == 1 ) // т.е. вторая переменная
                          {
                              // особый случай, если для характеристики указан порядок полинома
                              // тогда у предыдущей добавленной переменной нужно заменить Description
                              //
                              RenewVariables.last().Description = rVar.Description;

                          }

                          rVar.DeleteFlg     = true; // если вторая (и далее) переменная, то она не нужна
                          rVar.SfdObjectID   = "------------------------------------------";
                      }


                      break;
                  }

             }
             mdIndx++;
        }

        // пробуем найти эту переменную в образуемых характеристиках
        // и образовать полином через начальные значения
        int vri=0;
        while( VarsToCharacterRule[vri].FirstVar != "" )
        {
             for( int jIndx=0; jIndx < VarsToCharacterRule[vri].Vars.size(); jIndx++ )
             {
                  if( rVar.Name == VarsToCharacterRule[vri].Vars[jIndx] )
                  {
                      // если первая переменная, то вместо неё будем подставлять характеристику
                      if( jIndx == 0 ) // т.е. первая переменная
                      {
                          rVar.ObjectType   = CHTYPE_POLINOM;
                          rVar.sInitValue   = "-----------";
                          rVar.Substitution = SUBST_SOURCE_TECHNICAL_CONDITIONS;
                          rVar.SfdObjectID  = QString( JSON_TECHNICAL_CONDITIONS ) + "." + VarsToCharacterRule[vri].CharactId;// уникальный идентификатор характеристики

                          // и копируем весь список переменных
                          rVar.PolinomVars  = VarsToCharacterRule[vri].Vars;

                      }
                      else
                      {
                          rVar.DeleteFlg     = true; // если вторая (и далее) переменная, то она не нужна
                          rVar.SfdObjectID   = "------------------------------------------";
                      }


                      break;

                  }

             }

             vri++;
        }



// sfd_var_bd.txt - ОПИСАНИЕ АББРЕВИАТУР ПАРАМЕТРОВ МОДЕЛИ
// файл создан совместно с Мартиросовым март 2018г.
// Индексы перед идентификаторами параметра (переменной):
//  1- Переменная рассчитывается вне системы уравнений
//  2- Переменная рассчитывается как неизвестная в системе уравнений
// 21- Переменная рассчитывается как неизвестная в системе уравнений и измеряется
//  3- Константа из ТУ
// 31- Константа из автономных испытаний
//  4- Измеренные замыкающие парамеры системы уравнений

        if( rVar.DeleteFlg != true && rVar.Substitution == 0 )
        {
             if( rVar.TypeV == 0 )
             {
                  rVar.Substitution = 0;
                  rVar.SfdObjectID  = QString( "******** ERROR ************" );

             }

             if( rVar.TypeV == 1 )
             {
                  rVar.Substitution = SUBST_SOURCE_BOUNDARY_OUT_OF_EQUATIONS;
                  rVar.SfdObjectID  = QString( JSON_BOUNDARY_OUT_OF_EQUATIONS ) + "." + rVar.ConvertName;

             }

             if( rVar.TypeV == 2 )
             {
                  rVar.Substitution = SUBST_SOURCE_BOUNDARY_OUT_OF_EQUATIONS;
                  rVar.SfdObjectID  = QString( JSON_CALC_UNKNOWN_VARIABLE  ) + "." + rVar.ConvertName;

             }


             if( rVar.TypeV == 21 )
             {
                  rVar.Substitution = SUBST_SOURCE_MEASURED_VARIABLE;
                  rVar.SfdObjectID  = QString( JSON_MEASURED_VARIABLE  ) + "." + rVar.ConvertName;

             }

             if( rVar.TypeV == 3 )
             {
                  rVar.Substitution = SUBST_SOURCE_TECHNICAL_CONDITIONS;
                  rVar.SfdObjectID  = QString( JSON_TECHNICAL_CONDITIONS ) + "." + rVar.ConvertName;

             }

             if( rVar.TypeV == 31 )
             {
                   // ничего не делаем. так как уже сделано
             }

             if( rVar.TypeV ==  4 )
             {
                   rVar.Substitution = SUBST_SOURCE_CLOSED_AND_MEASURED;
                   rVar.SfdObjectID  = QString( JSON_CLOSED_AND_MEASURED_VARIABLE ) + "." + rVar.ConvertName;
             }

             if( rVar.TypeV ==  5 )
             {
                   rVar.Substitution = SUBST_SOURCE_CONSTANT;
                   rVar.SfdObjectID  = QString( JSON_CONSTANT ) + "." + rVar.ConvertName;
             }

             if( rVar.TypeV == 6 )
             {
                   // ничего не делаем. так как это две переменные с указание степени полинома
             }

             if( rVar.TypeV == 7 )
             {
                   rVar.Substitution = SUBST_SOURCE_CONTROL_VARIABLE;
                   rVar.SfdObjectID  = QString( JSON_CONTROL_VARIABLE ) + "." + rVar.ConvertName;

             }

             if( rVar.TypeV == 8 )
             {
                   rVar.Substitution = SUBST_SOURCE_SELECT_CONSTANT;
                   rVar.SfdObjectID  = QString( JSON_SELECT_CONSTANT ) + "." + rVar.ConvertName;

             }

             if( rVar.TypeV == 9 )
             {
                   rVar.Substitution = SUBST_SOURCE_EXPERT_AMENDMENTS;
                   rVar.SfdObjectID  = QString( JSON_EXPERT_AMENDMENTS ) + "." + rVar.ConvertName;

             }

        }


        RenewVariables.append( rVar );

    }

    // надо взять начальные значения переменных. Правило следущее:
    // в блочной модели находим первую переменную. Её значение и будет искомым
    QList<TRenewVariable>::iterator itrV;
    // цикл по новому списку переменных
    for( itrV = RenewVariables.begin(); itrV != RenewVariables.end(); ++itrV )
    {
        bool FindFlg=false;
        // цикл по блочной модели
        QList<SFDBlock>::iterator iSfd;
        for( iSfd = SFDBlocks.begin(); iSfd != SFDBlocks.end(); ++iSfd )
        {
            SFDBlock blk;
            blk = *iSfd;
            for( int i = 0; i < blk.Vars.size(); ++i )
            {
                 TVariable Var = blk.Vars.at(i);
                 if( (*itrV).Name == Var.Name && (*itrV).ObjectType == CHTYPE_NUMBER )
                 {
                     (*itrV).sInitValue    = Var.sInitValue;
                     (*itrV).UnknownSign   = Var.UnknownSign;
                     (*itrV).PrintSign     = Var.PrintSign;
                     (*itrV).UnknownVarFlg = Var.UnknownVarFlg;
                     (*itrV).PrintFlg      = Var.PrintFlg;

                     FindFlg = true;
                     break;
                 }
            }
            if( FindFlg == true )
                break;

        }

    }

    // надо взять начальные значения для образуемых характеристик. Правило следущее:
    // в блочной модели находим первую переменную. Её значение и будет искомым.
    // цикл по новому списку переменных
    for( itrV = RenewVariables.begin(); itrV != RenewVariables.end(); ++itrV )
    {
        TRenewVariable reVar;
        reVar = *itrV;

        if( (*itrV).ObjectType == CHTYPE_POLINOM )
        {
            for( int chVar=0; chVar<(*itrV).PolinomVars.size(); chVar++ )
            {
                 // берем имя переенной
                 QString vFind = (*itrV).PolinomVars.at( chVar );

                 // начинаем её поиск
                 // цикл по блочной модели
                 QList<SFDBlock>::iterator iSfd;
                 for( iSfd = SFDBlocks.begin(); iSfd != SFDBlocks.end(); ++iSfd )
                 {
                     SFDBlock blk;
                     blk = *iSfd;
                     // цикл по переменным блока
                     for( int i = 0; i < blk.Vars.size(); ++i )
                     {
                          TVariable Var = blk.Vars.at(i);
                          if( vFind == Var.Name )
                          {
                               //reVar.PolinomValue.append( Var.sInitValue );
                               //reVar.PolinomValue.append( "ddd " );
                               (*itrV).PolinomValue.append( Var.sInitValue );
// !!! очень странное поведение метода append() если стоит break; append не добавляет
                                //break;
                          }

                     }
                 }

            }
        }

    }


    // делаем проверку у всех переменных в модели значения одинаковые
    // цикл по переменным
    int DiscrepancyQnty = 0;
    for( itrV = RenewVariables.begin(); itrV != RenewVariables.end(); ++itrV )
    {
        bool FindFlg=false;
        QList<SFDBlock>::iterator iSfd;
        for( iSfd = SFDBlocks.begin(); iSfd != SFDBlocks.end(); ++iSfd )
        {
            SFDBlock blk;
            blk = *iSfd;
            for( int i = 0; i < blk.Vars.size(); ++i )
            {
                 TVariable Var = blk.Vars.at(i);
                 if( (*itrV).Name == Var.Name )
                 {
                     if( (*itrV).sInitValue != Var.sInitValue )
                     {
                          qDebug() << Var.Name << "Error расхождения в начальных значениях";
                          DiscrepancyQnty++;
                     }

                     if( (*itrV).UnknownVarFlg != Var.UnknownVarFlg )
                     {
                          qDebug() << Var.Name << "Error расхождения в UnknownVarFlg";
                     }

                 }
            }

        }

    }

    if( DiscrepancyQnty == 0 )
    {

        qDebug() << "Расхождений в начальных значениях не обнаружено";
        qDebug() << "---------------------------------------------------------------------------------------------";
        qDebug() << "---------------------------------------------------------------------------------------------";
    }

    //выводим на печать
    int OderNum=1;
    //QList<TRenewVariable>::iterator itrV;
    for( itrV = RenewVariables.begin(); itrV != RenewVariables.end(); ++itrV )
    {

             TRenewVariable reVar;
             reVar = *itrV;
             QString DeleteFlg;
             QString FindFlg;


             if( reVar.DeleteFlg   == true ) DeleteFlg   = "D"; else DeleteFlg   = " ";
             if( reVar.GuideFlg    == true ) FindFlg     = " "; else FindFlg     = "N";


             if( reVar.DeleteFlg   != true )
             {
             //qDebug("%3d   %3d  %s  %-26s %-48s %s", OderNum, reVar.TypeV, DeleteFlg.toUtf8().data(), reVar.ConvertName.toUtf8().data(), reVar.SfdObjectID.toUtf8().data(), reVar.Description.toUtf8().data() );
             qDebug("%3d %s %3d  %s  %-26s   %-15s   %-48s %s", OderNum, FindFlg.toUtf8().data(), reVar.TypeV, DeleteFlg.toUtf8().data(), reVar.Name.toUtf8().data(), reVar.sInitValue.toUtf8().data() , reVar.SfdObjectID.toUtf8().data(), reVar.Description.toUtf8().data() );
             if( reVar.ObjectType == CHTYPE_POLINOM )
             {
                qDebug() << reVar.PolinomVars;
                qDebug() << reVar.PolinomValue;
             }

             //if( reVar.Substitution != 0 && reVar.DeleteFlg != true )
             //       qDebug("                                                  %-s", reVar.SfdObjectID.toUtf8().data() );

             OderNum++;
             }

    }


    qDebug() << "---------------------------------------------------------------------------------------------";
    qDebug() << "---------------------------------------------------------------------------------------------";

}

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
void PrintBlockModel()
{

    int Qnty=1;
    int TypeCaseQnty=0;
    qDebug() << "---------------------------------------------------------------------------------------------";
    qDebug() << "---------------------------------------------------------------------------------------------";

    QList<SFDBlock>::iterator iSfd;
    for (iSfd = SFDBlocks.begin(); iSfd != SFDBlocks.end(); ++iSfd)
    {
        SFDBlock blk;
        blk = *iSfd;

        // инкрементируем счетчик использования Case
        UsedCase[ blk.CaseID ]++;

        qDebug() << "---------------------------------------------------------------------------------------------";
        // qDebug() << Qnty << "  CaseID:" << blk.CaseID << "  Equations: " << blk.Equations.size() << "  Vars: " << blk.Vars.size() << "Name: " << blk.Description;

        qDebug() << Qnty;
        qDebug() << "CaseID:         " << blk.CaseID;
        qDebug() << "Name:           " << blk.Description;
        qDebug() << "OrderNum 'b':   " << blk.OldOrderNum;
        qDebug() << "Equations Qnty: " << blk.Equations.size();
        qDebug() << "Vars Qnty:      " << blk.Vars.size();

        for (int i = 0; i < blk.Equations.size(); ++i)
             qDebug() << "EQU:" << blk.Equations.at(i);


        for (int i = 0; i < blk.Vars.size(); ++i)
        {
             TVariable Var = blk.Vars.at(i);

             QString Unk,Prt,Subst;
             if( Var.PrintFlg      == true ) Prt   = "Y"; else Prt   = " ";
             if( Var.UnknownVarFlg    == true ) Unk   = "Y"; else Unk   = " ";
             if( Var.Substitution  != 0    ) Subst = QString("%1").arg(Var.Substitution); else Subst = " ";

             qDebug("VAR: %3d Unk: %s  Prt: %s Sbt: %s  Tp: %2d   %15le    %-26s - %s", Var.OrdinalNum, Unk.toUtf8().data() , Prt.toUtf8().data(), Subst.toUtf8().data() , Var.TypeV, Var.InitValue, Var.Name.toUtf8().data(), Var.Description.toUtf8().data() );

             //qDebug() << "VAR:" << blk.Vars.at(i).Description;
        }



        Qnty++;
    }


#ifdef PRINT_UsedCase
    qDebug() << "---------------------------------------------------------------------------------------------";
    for( int i=1;i<MAX_SFD_CASE_ID+1; i++ )
    {

        if( UsedCase[i] > 0 )
        {
            qDebug() << "CaseId: " << i << "  CallQnty: " << UsedCase[i];
            TypeCaseQnty++;
        }

    }
    qDebug() << "TypeCaseQnty: " << TypeCaseQnty;
#endif

}

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
void PrepareNewBlockModel()
{
    // 1. нужно заменить переменные(которые содержат коэффициенты) на характеристику как объект
    //    под это подходит правило, что если переменная встречается в MdiUnitRule, то эту переменную исключить.
    //
    QList<SFDBlock>::iterator iSfd;
    for( iSfd = SFDBlocks.begin(); iSfd != SFDBlocks.end(); ++iSfd )
    {
        SFDBlock blk;
        blk = *iSfd;

        for( int i = 0; i < blk.Vars.size(); ++i )
        {
             TVariable Var = blk.Vars.at(i);

             QString Unk,Prt;

             if( Var.PrintFlg   == true ) Prt = "Y"; else Prt = " ";
             if( Var.UnknownVarFlg == true ) Unk = "Y"; else Unk = " ";

             qDebug("VAR: %3d Unk: %s  Prt: %s   Tp: %2d   %15le    %-26s - %s", Var.OrdinalNum, Unk.toUtf8().data() , Prt.toUtf8().data(), Var.TypeV, Var.InitValue, Var.Name.toUtf8().data(), Var.Description.toUtf8().data() );

             //qDebug() << "VAR:" << blk.Vars.at(i).Description;
        }

    }

}


/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
QJsonObject GetSfdObjectfromRenewVariable( TRenewVariable *rnVar )
{
    QJsonObject SfdObject;

    if( rnVar->ObjectType == CHTYPE_NUMBER )
    {
      SfdObject.insert( jSfdObjectKeyType,  CHTYPE_NUMBER       );
      SfdObject.insert( jSfdObjectKeyValue, rnVar->sInitValue    );
      SfdObject.insert( jSfdObjectKeyId,    rnVar->SfdObjectID   );

    }

    if( rnVar->ObjectType == CHTYPE_POLINOM )
    {

       QString PolinomValue;
       for (int i = 0; i < rnVar->PolinomValue.size(); ++i)
           PolinomValue += rnVar->PolinomValue.at(i) + "  ";

      SfdObject.insert( jSfdObjectKeyType,  CHTYPE_POLINOM       );
      SfdObject.insert( jSfdObjectKeyValue, PolinomValue         );
      SfdObject.insert( jSfdObjectKeyId,    rnVar->SfdObjectID   );

    }

    return SfdObject;

}

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
void MakeJsonNewBlockModel()
{

    int EqId=1;
    QList<SFDBlock>::iterator iSfd;
    for( iSfd = SFDBlocks.begin(); iSfd != SFDBlocks.end(); ++iSfd )
    {
        QJsonObject jOneMBlock;
        QJsonArray  jBlockVars;
        QJsonObject jEqu;
        QJsonArray  jEquations;

        SFDBlock blk;
        blk = *iSfd;

        for( int i = 0; i < blk.Vars.size(); ++i )
        {
            TVariable blkVar = blk.Vars.at(i);

            QList<TRenewVariable>::iterator itrV;
            for( itrV = RenewVariables.begin(); itrV != RenewVariables.end(); ++itrV )
            {

                 QJsonObject OneVar;
                 TRenewVariable reVar;
                 reVar = *itrV;
                 if( blkVar.Name == reVar.Name && reVar.DeleteFlg != true )
                 {
                     OneVar.insert( "Name",         reVar.Description );
                     OneVar.insert( jSfdObjectLink, reVar.SfdObjectID );
                     OneVar.insert( "OldVarName",   reVar.Name        );

                     jBlockVars.append( OneVar );
                 }

            }


        }

        // в исходной блочной модели все уравнения уникальны (т.е. нет повторяющихся)
        // поэтому можно сразу создать и json-раздел
        // Equations
        for( int iEq = 0; iEq < blk.Equations.size(); ++iEq )
        {

             QJsonObject EqObject;
             QJsonObject SfdObject;
             QString     EquationID;

             EquationID =  "Equation.Num"+QString::asprintf( "%03d", EqId );
             EqId++;

             jEqu.insert( "Name", blk.Equations.at(iEq) );
             jEqu.insert( jSfdObjectLink, EquationID );
             jEquations.append( jEqu );


             //EquationID = ;
             SfdObject.insert( jSfdObjectKeyType,      CHTYPE_NUMBER          );
             SfdObject.insert( jSfdObjectKeyValue,      ""                    );    // значение не задается
             SfdObject.insert( jSfdObjectKeyDecription, blk.Equations.at(iEq) );
             SfdObject.insert( jSfdObjectKeyId,     EquationID  ); // придумать

             EqObject.insert( jSfdObject, SfdObject );

             jAllEquations.insert( blk.Equations.at(iEq), EqObject );

        }

        jOneMBlock.insert( "Variables", jBlockVars );
        jOneMBlock.insert( "Equations", jEquations );
        jOneMBlock.insert( "CaseId",    blk.CaseID );

        jRenewBlockModel.insert( blk.Description, jOneMBlock );

    }

    jSfdConfig.insert("Конфигурация блочной модели", jRenewBlockModel );

    qDebug() << "---------------------------------------------------------------------------------------------";
    qDebug() << "---------------------------------------------------------------------------------------------";
    //qDebug() << "jRenewBlockModel:\n" << ((const char*)QJsonDocument(jRenewBlockModel).toJson(QJsonDocument::Compact));

}

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
void MakeJsonSectionSystemEquation()
{
    int iTs=0;
    QJsonObject jSystemEquation;

    while( TypeToSection[iTs].GuideType != 0 )
    {
        if( TypeToSection[iTs].ConvFlg == true )
        {

            QJsonObject OneSection;
            QList<TRenewVariable>::iterator itrV;
            //qDebug() << "MakeJsonSystemEquation" << TypeToSection[iTs].SectionName;

            // цикл по новому списку переменных
            for( itrV = RenewVariables.begin(); itrV != RenewVariables.end(); ++itrV )
            {
                 QJsonObject SfdObject;
                 QJsonObject VarObject;
                 TRenewVariable reVar;
                 reVar = *itrV;
                 if( TypeToSection[iTs].GuideType == reVar.TypeV && reVar.DeleteFlg != true )
                 {


                     /*if( reVar.ObjectType == CHTYPE_NUMBER )
                     {
                          SfdObject.insert( jSfdObjectKeyType,  CHTYPE_NUMBER       );
                          SfdObject.insert( jSfdObjectKeyValue, reVar.sInitValue    );
                          SfdObject.insert( jSfdObjectKeyId,    reVar.SfdObjectID   );

                          VarObject.insert( "OldVars",          reVar.Name );
                          VarObject.insert( jSfdObject, SfdObject );
                          OneSection.insert( reVar.Description, VarObject );
                     }*/
                     VarObject.insert( "OldVars",          reVar.Name );
                     VarObject.insert( jSfdObject, GetSfdObjectfromRenewVariable( &reVar ) );
                     OneSection.insert( reVar.Description, VarObject );

                 }
            }

            jSystemEquation.insert( TypeToSection[iTs].SectionName, OneSection );
        }

        iTs++;

    }

    // добавляем созданные ранее уравнения
    jSystemEquation.insert( "Уравнения", jAllEquations );

    // заносим в общую конфигурацию
    jSfdConfig.insert( "Система уравнений", jSystemEquation );

}

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
void MakeJsonTechnicalConditions()
{
    int iTs=0;
    QJsonObject jSystemEquation;

    while( TypeToSection[iTs].GuideType != 0 )
    {
        if( TypeToSection[iTs].GuideType == 3 ) break;
        iTs++;
    }

    // далее как в пребыдущей ф-ции
    QJsonObject OneSection;
    QList<TRenewVariable>::iterator itrV;
    qDebug() << "MakeJsonSystemEquation" << TypeToSection[iTs].SectionName;

    // цикл по новому списку переменных
    for( itrV = RenewVariables.begin(); itrV != RenewVariables.end(); ++itrV )
    {
         QJsonObject SfdObject;
         QJsonObject VarObject;
         TRenewVariable reVar;
         reVar = *itrV;
         if( TypeToSection[iTs].GuideType == reVar.TypeV && reVar.DeleteFlg != true )
         {
             /*if( reVar.ObjectType == CHTYPE_NUMBER )
             {
                  SfdObject.insert( jSfdObjectKeyType,  CHTYPE_NUMBER       );
                  SfdObject.insert( jSfdObjectKeyValue, reVar.sInitValue    );
                  SfdObject.insert( jSfdObjectKeyId,    reVar.SfdObjectID   );

                  VarObject.insert( "OldVars",          reVar.Name );
                  VarObject.insert( jSfdObject, SfdObject );
                  OneSection.insert( reVar.Description, VarObject );
             }*/
             VarObject.insert( "OldVars",          reVar.Name );
             VarObject.insert( jSfdObject, GetSfdObjectfromRenewVariable( &reVar ) );
             OneSection.insert( reVar.Description, VarObject );

         }
    }


    jSfdConfig.insert( TypeToSection[iTs].SectionName, OneSection );

}
/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
// 'b'
// файл 'b'   - исходная блочная модель (основной файл)
// содержит строки двух видов
// первый вид:
//  32   8   1   Температура горючего в СГ камеры
//  32 - порядковый номер
//   8 - номер блока обработки "case()"
//   1 - количество уравнений
// второй вид:
//        1    1 2.62591D+02      P к
//        1    2 2.75000D+00      Km к
//   1        11 1.00000D+00      adT сг к
//  1(поз.8) - у второго вида - неизвестная в системе уравнений
//  1(поз.3) - (для adT сг к) признак печати в выходной файл

// 'str'
// файл 'str' - описатель свойств case() блоков ( по фортрану )
// (по аналогии с СУР, это справочник описания аргументов ФП )
// вид описания строки:
// id-номер блока   - id-функции (номер case() - по фортрану )
// Число уравнений  - количество входных элеметов массива F()
// Число переменных - количество входных элеметов массива Z()
// в целом файл str - бесполезен, т.к. функционально проверку
// входных данных (аргументов), должен делать сам case()
// итого блоков case(101), которые надо переписать = 101

// 'mc'
// файл mc - матрица связей, последовательно содержит описания всех блоков
// описание одного блока: его номер  и ссылки (как порядковый номер) на элементы Z(),
// полученные в соответствие с файлом-перечнем 'i'
// общее количество блоков, может использоваться как контрольное число.
// особенность описания блока: если количество символов в строке больше какого-то кол-ва,
// то следущая строка является продолжением, а идентификатора блока.обработки равен нулю.
// в целом файл тоже бесполезен, т.к. для его формирования достаточно разобрать файл 'b'

// 'eq'
// файл eq - наименования уравнений модели
// в целом файл тоже бесполезен, т.к. для его формирования достаточно разобрать файл 'b'


// 'mda'
// файл 'mda' - исходные данные автономных испытаний агрегатов двигателя
// можно пока не преобразовывать и использовать файл 'mdi'

// 'mdi'
// файл 'mdi' - данные автономных испытаний агрегатов, приведенные к размерности мат.модели
// а также с указанием связи с переменной блочной модели 'b'.
// фактически одна строка это запись вида: "константа","переменная блочной модели"

// 'smd_i'
// файл 'smd_i' используется при преобразовании размерностей характеристик агрегатов в размерности  шаблона характеристик модели
// 'mda' + 'smd_i' = 'mdi'

// 'pc_1'
// файл 'pc_1'. Связка датчика (как измеряемого параметра) с переменными.
// + доп.свойства
// столбец 'индекс датчика':
// если 1 - датчик исп. для замыкания сист. уравнения. (обязательн)
// если 0 - невязка разности для формирования измеренного и расчетного значения
//
// столбец "индекс размерности":
// 0 - ничего не значит,
// 1 - обороты м мин, об/сек,
// 2 - цельсия в кельвины

// 'tlm_f' - данный телеметрии после фильтрации
// первый столбец - это время
// к вопросу образования файла tlm_f
// файл получается путем вызова программы RunQQ ('SFD_TLM_B.EXE','SFD_TLM_B.DAT REGIM')
// с аргументом sfd_tlm_b.dat, в котором и указаны исходный файл 'Parametri.prn' и выходной 'tlm_f'


// 'REGIM'
// файл 'REGIM' - режимы испытания
// файл в каком-то странном нечитаемом формате

// 'CLC'
// файл 'CLC' - данные для вычислительных процедур

// 'TOL_DP_1' и 'TOL_DP_2'
// допуски на диагностические признаки


//
// sfd_var_bd.txt - ОПИСАНИЕ АББРЕВИАТУР ПАРАМЕТРОВ МОДЕЛИ
// файл создан совместно с Мартиросовым март 2018г.
// Индексы перед идентификаторами параметра (переменной):
//  1- Переменная рассчитывается вне системы уравнений
//  2- Переменная рассчитывается как неизвестная в системе уравнений
// 21- Переменная рассчитывается как неизвестная в системе уравнений и измеряется
//  3- Константа из ТУ
// 31- Константа из автономных испытаний
//  4- Измеренные замыкающие парамеры системы уравнений
//  9- Экспертные поправки

// http://www.lpre.de/energomash/RD-170/index.htm#turbopump
// https://latexbase.com/d/c1c9384b-491d-4b5f-9fc7-e775e36a1766
/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
//обозначения:
//CaseID          - идентификатор блока обработки case()
//Name            - имя блока, полученное как описание первого уравнения + добвалена строка "Block."
//OrderNum        - старый порядковый номер  первого уравнения в блоке
//Equations Qnty  - кол-во уравнений
//Vars Qnty       - кол-во переменных
//VAR             - обозначение переменной
//Unk             - признак неизвестной
//Prt             - признак печати
//Tp              - тип согласно файлу sfd_var_guide.txt

// aD г вх др- вх нг - поправка на удельный вес горючего магистали питания насоса второй ступени
// P газ ст вх гв    - давление газа статическое на входе в газовод
// T газ вых т       - температера газа на выходе турбины
// P газ ст вх гв    - см.выше
// T газ вых т       - см выше
// K т бно           - Коэффициент согласования моментов на бустерном насосе агрегата окислителя.   поправочный коэффициент на баланс моментов
// kp газ вх т бно   - показатель аддиабаты
// T газ вых т       - см.выше
// aRe т тна         - поправка на степень реактивности турбины


// 1985
// Расход через Т ТНА
// Температура на вых Т ТНА
// Степень реактивности Т ТНА
// Давление в зазоре Т ТНА
// U/Cad Т ТНА

// 1946
// Расход через Т БНАО
// Температура на вых Т БНАО
// Давление в зазоре Т БНАО
// Степень реактивности Т БНАО

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
int main()
{

    // sfd_var_bd - описание переменных (файл создан совместно с Мартиросовым)
    // файл исходно в кодировке utf-8
    if( !varbdFile.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        qDebug() << "error open file " << varbdFile.fileName();
        return 0;
    }


    // блочная модель
    if( !BlkModel.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        qDebug() << "error open file " << BlkModel.fileName();
        return 0;
    }

    // данные агрегатов двигателя
    // исходный файл это файл 'mdi'
    if( !mdiFile.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        qDebug() << "error open file " << varbdFile.fileName();
        return 0;
    }

    InitRusToEng();

    // очищаем статистику используемых case
    for( int i=0;i<MAX_SFD_CASE_ID;i++ ) UsedCase[i]=0;

    // считываем-готовим guide-переменных
    PrepareVariableGuide();
    PrintVarGuide( 1  );
    PrintVarGuide( 2  );
    PrintVarGuide( 21 );
    PrintVarGuide( 3  );
    PrintVarGuide( 31 );
    PrintVarGuide( 4  );
    PrintVarGuide( 6  );
    PrintVarGuide( 7  );

    // подготавливаем свертку переменных образующих характеристику
    GroupVarsToCharacter();

    // реконструируем существующую блочную модель
    PrepareBlockModel();
    PrintBlockModel();

    PrepareRenewVariable();

    // считываем исходные данные из файла автономных испытаний mdi
    ReadUnitData();

    // реконструируем данные автономных испытаний, как характеристики
    // и формируем json object
    JsonUnitsData();

    // подготавливаем новую блочную модель
    // PrepareNewBlockModel();
    MakeJsonNewBlockModel();
    MakeJsonSectionSystemEquation();
    MakeJsonTechnicalConditions();


    QFile SfdCfgSmallFile( "/home/dmitry/energom/sfd/bin/debug/cfgsfd/jsn-sfdcfg-small.txt" );
    QFile SfdCfgBigFile(   "/home/dmitry/energom/sfd/bin/debug/cfgsfd/jsn-sfdcfg-big.txt" );

    if( SfdCfgSmallFile.open( QIODevice::WriteOnly )  && SfdCfgBigFile.open( QIODevice::WriteOnly ) )
    {
           SfdCfgSmallFile.write( QJsonDocument(jSfdConfig).toJson(QJsonDocument::Compact) );
           SfdCfgBigFile.write(   QJsonDocument(jSfdConfig).toJson(QJsonDocument::Indented) );

           SfdCfgSmallFile.close();
           SfdCfgBigFile.close();


    }
    else
       qDebug() << "\nError open file: " << SfdCfgSmallFile.error();


//    qDebug() << "new document:\n" << endl << ((const char*)QJsonDocument(jSfdConfig).toJson(QJsonDocument::Compact));

    return(0);

}


// "StandAloneData":{}
// "BPUFuel.PressureСharact":{ "sfdobject": }
// sfdobject: { id:"уникальный в рамках конфиги", description:"", type: "один коэфф-т или полином", value:{Koef:"", table:[], polinom:{}} }

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/

