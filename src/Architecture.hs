module Architecture where

import ModuleDef
import qualified Data.Map as M
import qualified Data.Vector as V
import Control.Monad

data System = System !(M.Map Int Module) !(V.Vector (Int,Int))

create :: [(Int, Module)] -> System
create xs = System (M.fromList xs) V.empty



perModule :: ((Int, Module) -> IO ()) -> System -> IO ()
perModule cb (System m v) = do
    forM_ (M.toList m) $ \m -> 
        cb m

rootInit :: System -> IO ()
rootInit = perModule (\(_,Module g _) -> root_init g)

normalInit :: System -> IO ()
normalInit = perModule (\(_,Module g _) -> user_init g)
