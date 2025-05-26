#include <QApplication>
#include <iostream>
#include "ui/main_window.h"
#include "models/simulator.h"

int main(int argc, char *argv[])
{
    try {
        QApplication app(argc, argv);
        
        qRegisterMetaType<trade_simulator::models::SimulatorOutput>("models::SimulatorOutput");
        
        QApplication::setApplicationName("Trade Simulator");
        QApplication::setApplicationVersion("1.0.0");
        
        trade_simulator::ui::MainWindow mainWindow; 
        mainWindow.show(); 
        
        return app.exec();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
} 