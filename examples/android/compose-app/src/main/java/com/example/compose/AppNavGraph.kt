package com.example.compose

import androidx.compose.runtime.Composable
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.example.compose.ui.screens.DetailScreen
import com.example.compose.ui.screens.HomeScreen

private const val ROUTE_HOME = "home"
private const val ROUTE_DETAIL = "detail"

@Composable
fun AppNavGraph() {
    val navController = rememberNavController()
    NavHost(navController = navController, startDestination = ROUTE_HOME) {
        composable(ROUTE_HOME) {
            HomeScreen(onNavigateToDetail = { navController.navigate(ROUTE_DETAIL) })
        }
        composable(ROUTE_DETAIL) {
            DetailScreen(onNavigateBack = { navController.popBackStack() })
        }
    }
}
