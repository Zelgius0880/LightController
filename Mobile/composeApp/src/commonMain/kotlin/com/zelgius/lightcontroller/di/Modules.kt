package com.zelgius.lightcontroller.di

import com.zelgius.lightcontroller.platformModule
import org.koin.core.annotation.KoinApplication
import org.koin.dsl.KoinAppDeclaration
import org.koin.dsl.includes
import org.koin.plugin.module.dsl.startKoin

@KoinApplication(modules = [RepositoryModule::class, ViewModelModule::class])
class App

fun initKoin(appDeclaration: KoinAppDeclaration = {}) =
    startKoin<App>{
        includes(appDeclaration)
        modules(platformModule )
    }
