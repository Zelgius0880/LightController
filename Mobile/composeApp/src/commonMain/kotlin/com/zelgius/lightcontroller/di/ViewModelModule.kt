package com.zelgius.lightcontroller.di

import org.koin.core.annotation.ComponentScan
import org.koin.core.annotation.Module

@Module(includes = [RepositoryModule::class])
@ComponentScan(
    "com.zelgius.lightcontroller.ui",

    )
class ViewModelModule